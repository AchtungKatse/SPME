#include "FileTypes/LevelGeometry/GeometryExporter.h"
#include "FileTypes/LevelGeometry/MapStructures.h"
#include "FileTypes/MapConfig.h"
#include "FileTypes/TPL.h"
#include "Types/Types.h"
#include "core/Logging.h"
#include "assimp/material.h"
#include "assimp/types.h"
#include "assimp/vector3.h"
#include <algorithm>
#include <fstream>
#include <ios>
#include <numbers>
#include <string>
#include "stb_image.h"

using namespace SPMEditor::MapStructures;
namespace SPMEditor {
    GeometryExporter* GeometryExporter::Create() { 
        return new GeometryExporter();
    }

    GeometryExporter::GeometryExporter() : mVCDTable({}) {
        memset(&mVCDTable, 0, sizeof(VCDTable));
    }
    GeometryExporter::~GeometryExporter() {
        delete mData;
        delete mTextBuffer;
    };

    GeometryExporter::Section::Section(const char* name, int offset) : name(name), offset(offset) { }

    void GeometryExporter::Write(const aiScene* scene, const MapConfig config, const std::string& outputDirectory) {
        mScene = scene;
        mMapConfig = config;
        
        // Create paths for output files
        char outputMapPath[1024];
        char outputTplPath[1024];

        snprintf(outputMapPath, sizeof(outputMapPath), "%s/dvd/map/%s/map.dat", outputDirectory.c_str(), config.mMapName.c_str());
        snprintf(outputTplPath, sizeof(outputTplPath), "%s/dvd/map/%s/texture.tpl", outputDirectory.c_str(), config.mMapName.c_str());

        // For whatever reason, blender's gltf export results in all images having the name "Image-Image" which seems to confuse SPM
        // HACK: the fix for this is just going to be to append image indices to their names
        // NOTE: This doesn't seem to actually affect the textures used since materials reference it via an ImageInfo*
        for (size_t i = 0; i < scene->mNumTextures; i++) {
            scene->mTextures[i]->mFilename.Append("_index_");
            scene->mTextures[i]->mFilename.Append(std::to_string(i).c_str());
        }


        // Write the scene images as a TPL
        LogWarn("Writing tpl to '%s'", outputTplPath);
        WriteTPL(outputTplPath);

        std::ofstream output(outputMapPath);

        mFileSize = 0;
        mTextBufferSize = 0;

        constexpr int DataBufferSize = 1024 * 1024 * 50; // 50MB
        constexpr int TextBufferSize = 1024 * 1024 * 10; // 10MB
        mData = new u8[DataBufferSize]; // Overly large buffer
        mTextBuffer = new char[TextBufferSize];
        memset(mData, 0, DataBufferSize);

        WriteInfoSection();
        WriteMapTextures();
        WriteVertexData(6, 8);
        AddPadding(0x20);
        WriteMaterials();
        WriteMeshes();
        AddPadding(0x20);
        WriteObjects();
        WriteVCDTable();
        AddPadding(0x20);
        WriteMaterialTable();
        WriteLightTable();
        WriteFogTable();
        WriteTextureTable();
        WriteAnimationTable();

        // Fix all things that need pointers to other things
        for (size_t i = 0; i < mVCDPtrs.size(); i++) {
            *(int*)(mData + mVCDPtrs[i] + 0x20) = ByteSwap(mVCDAddress);
        }

        for (const auto& subdata: mObjectSubdatas) {
            int offset = AppendInt32(0);
            AppendInt32(0);
            AppendInt32(0);
            AppendInt32(0);
            AppendInt32(0);
            *(int*)(mData + subdata.offset + 0x20) = ByteSwap(offset);
        }

        for (const auto& subdata : mMaterialSubdatas) {
            int offset = AppendInt32(0);
            AppendInt32(0);
            AppendInt32(0);
            *(int*)(mData + subdata.offset + 0x20) = ByteSwap(offset);
        }

        *(int*)(mData + 0x4 + 0x20) = ByteSwap(mRootObjectPtr);


        AddPadding(0x4);

        for (size_t stringPointer : mStringPointers) {
            *(int*)(mData + stringPointer + 0x20) = ByteSwap(ByteSwap(*(int*)(mData + stringPointer + 0x20)) + mFileSize);
        }
        AddPadding(0x4);
        AppendBuffer(mTextBuffer, mTextBufferSize);
        AddPadding(0x4);

        // Write Fat
        for (const int address : mPointerList) {
            AppendInt32(address);
        }

        WriteHeader();

        // Write to disk
        output.write((const char*)mData, mFileSize);
        WriteSectionTable(output);
        size_t fileSize = ByteSwap((int)output.tellp());
        output.seekp(std::ios::beg);
        output.write((const char*)&fileSize, 4);
    }

    void GeometryExporter::WriteHeader() { 
        FileHeader* fileHeader = (FileHeader*)mData;
        mFileSize += sizeof(FileHeader); // Added at the end because pointer magic

        /*fileHeader->fileLength = mFileSize + mTextBufferSize;*/
        fileHeader->pointerListStart = mFileSize - sizeof(FileHeader) - 0x4 * mPointerList.size();
        fileHeader->pointerEntryCount = mPointerList.size();
        fileHeader->sectionCount = mSections.size();
        ByteSwap4(fileHeader, 4);
    }

    void GeometryExporter::WriteInfoSection() {
        int startOffset = AppendStringPointer("ver1.02");
        AppendPointer(0);
        AppendStringPointer("S");
        AppendStringPointer("A");
        AppendStringPointer("07/02/26 17:09:56");
        mSections.emplace_back(Section("information", startOffset));
    }

    void GeometryExporter::WriteMapTextures() {
        LogInfo("Writing %d textures", mScene->mNumTextures);
        for (size_t i = 0; i < mScene->mNumTextures; i++) {
            // Create map texture
            const aiTexture* texture = mScene->mTextures[i];
            MapTexture mapTexture;
            mapTexture.nameOffset = AppendStringPointer(texture->mFilename.C_Str());

            mapTexture.transparency = MapTexture::TransparencyType::Opaque;
            if (texture->mHeight != 0) {
                mapTexture.width = texture->mWidth;
                mapTexture.height = texture->mHeight;
            } else {
                // Run stbi_load_from_memory to get real image dimensions
                int channels = 0;
                Color* colors = (Color*)stbi_load_from_memory((u8*)texture->pcData, texture->mWidth, (int*)&mapTexture.width, (int*)&mapTexture.height, &channels, 0);
                stbi_image_free(colors);

            }
            // TODO: Research other transparency modes
            TextureConfig config = GetTextureConfig(i);
            mapTexture.transparency = (config.mUseTransparency ? MapTexture::TransparencyType::Clip : MapTexture::TransparencyType::Opaque);

            mTextureNameTable.emplace_back(mapTexture.nameOffset);
            mTextureNameToIndex.emplace(texture->mFilename.C_Str(), mTextureNameToIndex.size());
            LogInfo("Adding texture name '%s' at index %u", texture->mFilename.C_Str(), mTextureNameToIndex.size());

            // Everything else is unkown
            AppendInt8((s8)mapTexture.transparency);
            AppendInt8(0);
            AppendInt8(0);
            AppendInt8(0);

            AppendInt16(mapTexture.width);
            AppendInt16(mapTexture.height);
            AppendInt32(0);
        }

        // Write information
        for (size_t i = 0; i < mScene->mNumTextures; i++) {
            TextureConfig config = GetTextureConfig(i);
            AppendPointer(0x14 + i * sizeof(MapTexture));
            AppendInt32(0); // padding
            AppendInt8((u8)config.mWrapModeU);
            AppendInt8((u8)config.mWrapModeV);
            AppendInt8(0);
            AppendInt8(0);
        }
    }

    void GeometryExporter::WriteSectionTable(std::ofstream& output) {
        int nameOffset = 0;
        int nameBufferSize = 0;
        for (size_t i = 0; i < mSections.size(); i++) {
            nameBufferSize += 1 + strlen(mSections[i].name);
        }

        char nameBuffer[0x400];
        Assert((u64)nameBufferSize < sizeof(nameBuffer), "GeometryExporter cannot write section table, section name table size is greater than allocated name buffer size (%d >= %d)", nameBufferSize, sizeof(nameBuffer));

        LogInfo("Writing name buffer of size: 0x%x", nameBufferSize);

        for (size_t i = 0; i < mSections.size(); i++) {
            strcpy(nameBuffer + nameOffset, mSections[i].name);

            int offset = ByteSwap(mSections[i].offset);
            int _nameOffset = ByteSwap(nameOffset);
            output.write((const char*)&offset, 4);
            output.write((const char*)&_nameOffset, 4);
            nameOffset += strlen(mSections[i].name) + 1;
        }

        output.write(nameBuffer, nameBufferSize);
    }

    void GeometryExporter::WriteVertexData(u8 vertexScale, u8 uvScale) {
        mVCDTable.vertexScale = vertexScale;
        mVCDTable.uvScale = uvScale;
        GetMeshDataRecursive(mScene->mRootNode);
        AddPadding(0x20);

        LogInfo("Writing 0x%x vertices", mVertexTable.size());
        mVCDTable.vertices = AppendInt32(mVertexTable.size()) + 4;
        mFileSize += mVertexTable.size() * 6;
        u32 _vertexScale = 1 << vertexScale;
        for (const auto pair : mVertexTable) {
            const auto v = pair.first;
            int vertexIndex = pair.second;
            vec3<s16>* vertex = &mVCDTable.vertices.Get(mData + 0x20)[vertexIndex];
            vertex->x = ByteSwap((s16)(v.x * _vertexScale));
            vertex->y = ByteSwap((s16)(v.y * _vertexScale));
            vertex->z = ByteSwap((s16)(v.z * _vertexScale));
        }

        AddPadding(0x20);
        mVCDTable.colors = AppendInt32(mColorTable.size()) + 4;
        mFileSize += mColorTable.size() * 4;
        for (const auto pair : mColorTable) {
            const auto color = pair.first;
            int index = pair.second;
            Color* outColor = &mVCDTable.colors.Get(mData + 0x20)[index];
            outColor->r = ByteSwap((u8)(color.r * 255));
            outColor->g = ByteSwap((u8)(color.g * 255));
            outColor->b = ByteSwap((u8)(color.b * 255));
            outColor->a = ByteSwap((u8)(color.a * 255));
        }

        AddPadding(0x20);
        mVCDTable.uvs = AppendInt32(mUvTable.size()) + 4;
        mFileSize += mUvTable.size() * 4;
        u32 _uvScale = 1 << uvScale;
        for (const auto pair : mUvTable) {
            const auto uv = pair.first;
            int index = pair.second;
            vec2<s16>* outUV = &mVCDTable.uvs.Get(mData + 0x20)[index];
            outUV->x = ByteSwap((s16)(uv.x * _uvScale));
            outUV->y = ByteSwap((s16)((1.0f - uv.y) * _uvScale)); // 1.0f - uv.y fixes textures from being upside down
        }

        AddPadding(0x20);
        mVCDTable.normals = AppendInt32(mNormalTable.size()) + 4;
        LogInfo("Writing %u normals.", mNormalTable.size());
        for (const auto pair : mNormalTable) {
            const auto normal = pair.first;
            int index = pair.second;
            vec3<s8>* outNormal = &mVCDTable.normals.Get(mData + 0x20)[index];
            outNormal->x = ByteSwap((u8)(normal.x * 64));
            outNormal->y = ByteSwap((u8)(normal.y * 64));
            outNormal->z = ByteSwap((u8)(normal.z * 64));
        }
        AddPadding(0x20);
    }

    void GeometryExporter::GetMeshDataRecursive(const aiNode* node) {
        for (size_t i = 0; i < node->mNumMeshes; i++) {
            const aiMesh* mesh = mScene->mMeshes[node->mMeshes[i]];
            for (size_t v = 0; v < mesh->mNumVertices; v++) {
                const auto vertex = mesh->mVertices[v];
                if (!mVertexTable.contains(vertex)) {
                    if (mVertexTable.size() == 0)
                        LogInfo("Adding vertex (%f, %f, %f) to vertex table at index %u", vertex.x, vertex.y, vertex.z, mVertexTable.size());
                    mVertexTable.emplace(vertex, mVertexTable.size());
                }
                if (mesh->HasVertexColors(0)) {
                    aiColor4D color = mesh->mColors[0][v];
                    if (!mColorTable.contains(color)) {
                        mColorTable.emplace(mesh->mColors[0][v], mColorTable.size());
                    }
                }

                if (mesh->mNormals) {
                    if (!mNormalTable.contains(mesh->mNormals[v])) {
                        mNormalTable.emplace(mesh->mNormals[v], mNormalTable.size());
                    }
                }

                if (mesh->HasTextureCoords(0)) {
                    if (!mUvTable.contains(mesh->mTextureCoords[0][v])) {
                        aiVector3D uv = mesh->mTextureCoords[0][v];
                        size_t index = mUvTable.size();
                        mUvTable.emplace(uv, index);
                    }
                }
            }

        }

        for (size_t i = 0; i < node->mNumChildren; i++) {
            GetMeshDataRecursive(node->mChildren[i]);
        }
    }

    void GeometryExporter::WriteMaterials() {
        for (size_t i = 0; i < mScene->mNumMaterials; i++) {
            aiMaterial* material = mScene->mMaterials[i];
            LogInfo("Adding material. Name: %s", material->GetName().C_Str());
            int address = AppendStringPointer(material->GetName().C_Str());
            int nameAddress = mStringTable[material->GetName().C_Str()];
            mMaterialTable.emplace_back(MaterialNameEntry{.nameOffset = nameAddress, .materialOffset = address});

            // Get the config
            MaterialConfig config = GetMaterialConfig(i);

            aiVector3D materialColor(1);
            float opacity = 0;
            if (AI_SUCCESS != material->Get(AI_MATKEY_BASE_COLOR, materialColor)) LogWarn("Failed to get diffuse color from material '%s'", material->GetName().C_Str());
            if (AI_SUCCESS != material->Get(AI_MATKEY_OPACITY, opacity)) LogWarn("Failed to get opacity from material '%s'", material->GetName().C_Str());

            AppendUInt8((u8)(materialColor.x * 255));
            AppendUInt8((u8)(materialColor.y * 255));
            AppendUInt8((u8)(materialColor.z * 255));
            /*AppendUInt8((u8)((1 - opacity) * 255));*/
            AppendUInt8(255);


            u8 useVertexColors = config.mUseVertexColor;
            u8 unk_1 = 1;
            u8 useTransparency = config.mUseTransparency;
            u8 useTexture = material->GetTextureCount(aiTextureType_DIFFUSE) > 0;
            AppendUInt8(useVertexColors);
            AppendUInt8(unk_1);
            AppendUInt8(useTransparency);
            AppendUInt8(useTexture);

            if (useTexture) {
                Assert(mScene->mNumTextures > 0, "Material uses textures but scene does not model does not contain any embedded textures");
                aiString* path = new aiString();
                Assert(aiReturn_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, path), "Failed to get texture for material '%s'", material->GetName().C_Str());
                u32 textureInfoPtr = 0x14 + mScene->mNumTextures * sizeof(MapTexture);
                if (mTextureNameToIndex.contains(path->C_Str())) {
                    textureInfoPtr += mTextureNameToIndex[path->C_Str()] * sizeof(MapTexture::Info);
                } else if (path->C_Str()[0] == '*') {
                    // Check if texture name is assimp using an image index
                    int imageIndex = std::stoi(path->C_Str() + 1);
                    textureInfoPtr += imageIndex * sizeof(MapTexture::Info);

                }
                else {
                    LogWarn("Texture Name To Index table does not have texture '%s'. Using first texture.", path->C_Str());
                    textureInfoPtr += sizeof(MapTexture) * (i % mScene->mNumTextures);
                }
                LogDebug("Material texture path '%s' has index of %d and offset of %p", path->C_Str(), mTextureNameToIndex[path->C_Str()], textureInfoPtr);
                AppendPointer(textureInfoPtr);
            } else {
                AppendInt32(0);
            }


            for (size_t a = 0; a < 9; a++) { // Parameters
                AppendFloat(0);
                AppendFloat(0);
                AppendFloat(1);
                AppendFloat(1);
                AppendFloat(0);
                AppendFloat(0);
                AppendFloat(0);
            }
            AppendInt32(0xFFFFFFFF); // Unk color
            int subdataOffset = AppendPointer(0x0); // Unk PTR

            mMaterialSubdatas.emplace_back(MaterialSubData{Material::SubData(), subdataOffset} );
        }
    }

    void GeometryExporter::WriteMeshes() {
        for (size_t i = 0; i < mScene->mNumMeshes; i++) {
            WriteMesh(mScene->mMeshes[i]);
        }
    }

    void GeometryExporter::WriteMesh(const aiMesh* mesh) {
        LogInfo("Writing mesh '%s'", mesh->mName.C_Str());
        // Get the vertex attribues
        int vertexSize = 2;
        VertexAttributes attr = VertexAttributes::VERTEX_ATTRIBUTE_POSITION;
        if (mesh->HasNormals()) {
            attr = (VertexAttributes)((int)attr | (int)VertexAttributes::VERTEX_ATTRIBUTE_NORMAL);
            vertexSize += 2;
        }
        if (mesh->HasVertexColors(0)) {
            attr = (VertexAttributes)((int)attr | (int)VertexAttributes::VERTEX_ATTRIBUTE_COLOR);
            vertexSize += 2;
        }
        if (mesh->HasTextureCoords(0)) {
            attr = (VertexAttributes)((int)attr | (int)VertexAttributes::VERTEX_ATTRIBUTE_UV); 
            vertexSize += 2;
        }

        std::vector<VertexStrip::Header> stripHeaders;
        if (!mesh->mNormals) {
            LogWarn("Mesh %s does not have normals", mesh->mName.C_Str());
        }
        if (!mesh->HasTextureCoords(0)) {
            LogWarn("Mesh %s does not have UVs", mesh->mName.C_Str());
        }
        // HACK: For the sake of making an easy implementation, each strip is just going to be one triangle
        // TODO: Add triangle stripping algorithm

        for (size_t f = 0; f < mesh->mNumFaces; f++) {
            int headerOffset = mFileSize;
            const aiFace face = mesh->mFaces[f];
            AppendUInt8(0x98); // some constant
            AppendInt16(3); // Vertex count, 3 since one triangle at a time

            // NOTE: this loop decrements i for face winding reasons
            for (uint i = face.mNumIndices - 1; i >= 0 && i != 0xffffffff; i--) {
                Assert(face.mNumIndices == 3, "Invalid num indices per face. Got %u, expected 3", face.mNumIndices);
                u32 index = face.mIndices[i];
                const aiVector3D vertex = mesh->mVertices[index];

                // Write the vertex index as a u16
                AppendInt16(mVertexTable[vertex]);
                if (mesh->mNormals) {
                    AppendInt16(mNormalTable[mesh->mNormals[index]]);
                }
                if (mesh->HasVertexColors(0)) {
                    AppendInt16(mColorTable[mesh->mColors[0][index]]);
                }
                if (mesh->HasTextureCoords(0)) {
                    aiVector3D uv = mesh->mTextureCoords[0][index];
                    Assert(mUvTable.contains(uv), "Mesh cannot use UV since it is not in uv table.");
                    AppendInt16(mUvTable[uv]);
                }
            }


            // Write strip header
            int size = 3 + vertexSize * 3;
            int padding = 0x20 - (size % 0x20);
            if (padding != 0x20) { // Round to nearest 0x20 bytes
                size += padding;
            }

            VertexStrip::Header header { 
                .entryOffset = headerOffset, 
                    .length = size,
            };
            stripHeaders.emplace_back(header);
            AddPadding(0x20);
        }

        // Write mesh header
        int address = AppendInt32(0x1000001);
        AppendInt32(stripHeaders.size());
        AppendInt32((u32)attr);
        mVCDPtrs.emplace_back(AppendPointer(0xFFFFFFFF));
        mMeshTable.emplace_back(address);

        for (const VertexStrip::Header& header : stripHeaders) {
            AppendPointer(header.entryOffset);
            AppendInt32(header.length);
        }
        AddPadding(0x20);
    }

    void GeometryExporter::WriteObjects() {
        mRootObjectPtr = WriteObject(mScene->mRootNode, 0, 0);
    }

    u32 GeometryExporter::WriteObject(const aiNode* node, u32 parentOffset, u32 previousSibling) {
        int selfAddress;
        if (strcmp(node->mName.C_Str(), "RootNode") == 0)
            selfAddress = AppendStringPointer("world_root");
        else 
            selfAddress = AppendStringPointer(node->mName.C_Str());
        LogInfo("Writing object '%s' at %d", node->mName.C_Str(), selfAddress);

        if (node->mNumMeshes > 0)
            AppendStringPointer("mesh");
        else AppendStringPointer("null");

        if (parentOffset)
            AppendPointer(parentOffset);
        else AppendInt32(0);

        int childPtrOffset = AppendInt32(0); // Temporary child offset
        AppendInt32(0); // Next sibling offset

        if (previousSibling)
            previousSibling = AppendPointer(previousSibling);
        else previousSibling = AppendInt32(0);

        aiVector3D position, rotation, scale;
        node->mTransformation.Decompose(scale, rotation, position);
        AppendVector3(scale);
        AppendVector3(rotation * 180.0f / (float)std::numbers::pi);
        AppendVector3(position);

        // Calculate mesh bounds
        aiVector3D boundsMin(10000000000000.0f);
        aiVector3D boundsMax(-1000000000000.0f);
        for (uint i = 0; i < node->mNumMeshes; i++) {
            const auto mesh = mScene->mMeshes[node->mMeshes[0]];
            for (uint v = 0; v < mesh->mNumVertices; v++) {
                auto vert = mesh->mVertices[v];

                if (vert.x < boundsMin.x) {
                    boundsMin.x = vert.x;
                }
                if (vert.y < boundsMin.y) {
                    boundsMin.y = vert.y;
                }
                if (vert.z < boundsMin.z) {
                    boundsMin.z = vert.z;
                }

                if (vert.x > boundsMax.x) {
                    boundsMax.x = vert.x;
                }
                if (vert.y > boundsMax.y) {
                    boundsMax.y = vert.y;
                }
                if (vert.z > boundsMax.z) {
                    boundsMax.z = vert.z;
                }
            }
        }
        AppendVector3(boundsMin);
        AppendVector3(boundsMax);

        AppendInt32(0);
        if (parentOffset == 0) { // Is root
            AppendInt32(0); // subdataptr
        } else {
            int subdataOffset = AppendPointer(0); // subdataptr
            mObjectSubdatas.emplace_back(ObjectSubData{Object::SubData(), subdataOffset});
            mPointerList.emplace_back(subdataOffset);

        }

        AppendInt32(node->mNumMeshes);

        // Add filler for no reason, might try to remove this later
        if (node->mNumMeshes <= 0) {
            AppendInt32(0);
            AppendInt32(0);
        }

        for (size_t i = 0; i < node->mNumMeshes; i++) {
            const aiMesh* mesh = mScene->mMeshes[node->mMeshes[i]];
            LogInfo("Mesh '%s' has material index %u ('%s')", node->mName.C_Str(), mesh->mMaterialIndex, mScene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str());
            AppendPointer(mMaterialTable[mesh->mMaterialIndex].materialOffset);
            AppendPointer(mMeshTable[node->mMeshes[i]]);
        }

        // Write the children
        // TODO: remove variable length array
        constexpr uint NODE_MAX_CHILD_COUNT = 0x1000;
        Assert(node->mNumChildren < NODE_MAX_CHILD_COUNT, 
                "Geometry Exporter failed to write object '%s', too many children (%d). Max number of children is %d. How did this even happen?", 
                node->mName.C_Str(), 
                node->mNumChildren, 
                NODE_MAX_CHILD_COUNT);

        Object* children[NODE_MAX_CHILD_COUNT];
        int childrenOffsets[NODE_MAX_CHILD_COUNT];

        u32 previousChild = 0;
        for (size_t i = 0; i < node->mNumChildren; i++) {
            int childOffset = WriteObject(node->mChildren[i], selfAddress, previousChild);
            previousSibling = childOffset;
            children[i] = (Object*)(mData + 0x20 + childOffset);
            childrenOffsets[i] = childOffset;
        }

        // Write children siblings
        for (size_t i = 0; i < node->mNumChildren; i++) {
            // Write next
            if (i < node->mNumChildren - 1) {
                children[i]->nextSibling = ByteSwap(childrenOffsets[i + 1]);
                AddPointer(childrenOffsets[i] + offsetof(Object, nextSibling));
            }
            // Write previous
            if (i > 0) {
                children[i]->previousSibling = ByteSwap(childrenOffsets[i - 1]);
                AddPointer(childrenOffsets[i] + offsetof(Object, previousSibling));
            }
        }

        if (node->mNumChildren > 0) {
            *(u32*)(mData + 0x20 + childPtrOffset) = ByteSwap(childrenOffsets[0]);
            AddPointer(childPtrOffset);
        }

        return selfAddress;
    }

    int GeometryExporter::GetObjectSize(const aiNode* node) {
        return sizeof(Object) + std::max(node->mNumMeshes, 1u) * 8;
    }

    void GeometryExporter::WriteVCDTable() {
        mVCDAddress = AppendPointer(mVCDTable.vertices.address - 4);
        AppendPointer(mVCDTable.normals.address - 4);
        AppendInt32(0);
        AppendPointer(mVCDTable.colors.address - 4);
        AppendInt32(mVCDTable.unknown_2);
        AppendInt32(mVCDTable.unknown_3);
        AppendPointer(mVCDTable.uvs.address - 4);
        AppendBuffer(mVCDTable.unknown_4, sizeof(mVCDTable.unknown_4));
        AppendInt32(mVCDTable.vertexScale);
        AppendInt32(mVCDTable.uvScale);
        mSections.emplace_back("vcd_table", mVCDAddress);
    }

    void GeometryExporter::WriteMaterialTable() {
        u32 address = AppendInt32(mMaterialTable.size());

        for (const MaterialNameEntry& entry : mMaterialTable) {
            u32 nameOffset = AppendPointer(entry.nameOffset);
            AppendPointer(entry.materialOffset);
            mStringPointers.emplace_back(nameOffset);
        }

        mSections.emplace_back(Section("material_name_table", address));
    }

    void GeometryExporter::WriteLightTable() {
        // Unsupported
        int address = AppendInt32(0);

        mSections.emplace_back(Section("light_table", address));
    }

    void GeometryExporter::WriteFogTable() {
        // Unsupported because its not in fbx files
        int address = AppendInt32(0);

        mSections.emplace_back(Section("fog_table", address));
    }

    void GeometryExporter::WriteTextureTable() {
        // Unsupported because its not in fbx files
        int address = AppendInt32(mTextureNameTable.size());

        for (size_t i = 0; i < mScene->mNumTextures; i++) {
            AppendStringPointer(mScene->mTextures[i]->mFilename.C_Str());
        }

        mSections.emplace_back(Section("texture_table", address));
    }

    void GeometryExporter::WriteAnimationTable() {
        // Unsupported
        int address = AppendInt32(0);

        mSections.emplace_back(Section("animation_table", address));
    }

    void GeometryExporter::WriteTPL(const char* outputPath) {
        constexpr u32 MAX_TEXTURE_COUNT = 1024;
        TPLImageCreateInfo imageCreateInfos[MAX_TEXTURE_COUNT];
        for (size_t i = 0; i < mScene->mNumTextures; i++) {
            imageCreateInfos[i].mFormat = TPLImageFormat::RGBA32;
            imageCreateInfos[i].mTexture = mScene->mTextures[i];
        }

        TPLCreateInfo createInfo = {
            .mImageCreateInfos = imageCreateInfos,
            .mImageCreateInfoCount = mScene->mNumTextures,
        };

        TPL tpl = TPL::CreateTPL(createInfo);
        tpl.Write(outputPath);
    }

    int GeometryExporter::AppendBuffer(void* buffer, int size) {
        int fileOffset = mFileSize;
        memcpy(mData + mFileSize + 0x20, buffer, size);
        mFileSize += size;
        return fileOffset;
    }

    int GeometryExporter::AddString(const char* text) {
        if (mStringTable.contains(text)) {
            return mStringTable[text];
        }

        int offset = mTextBufferSize;
        int size = strlen(text) + 1;
        memcpy(mTextBuffer + offset, text, size);
        mTextBufferSize += size;

        // Add to string table
        mStringTable.emplace(text, offset);
        return offset;
    }

    int GeometryExporter::AppendStringPointer(const char* text) {
        int offset = AddString(text);

        int fileOffset = AppendPointer(offset);
        mStringPointers.emplace_back(fileOffset);

        return fileOffset;
    }

    int GeometryExporter::AppendPointer(const s32 value) {
        int offset = _AppendInt(value);
        mPointerList.emplace_back(offset);
        return offset;
    }

    void GeometryExporter::AddPointer(const s32 address) {
        mPointerList.emplace_back(address);
    }

    int GeometryExporter::AppendInt32(const s32 value) {
        return _AppendInt(value);
    }

    int GeometryExporter::AppendInt16(const s16 value) {
        return _AppendInt(value);
    }

    int GeometryExporter::AppendInt8(const s8 value) {
        int fileOffset = mFileSize;
        *(s8*)(mData + mFileSize + 0x20) = value;
        mFileSize++;
        return fileOffset;
    }

    int GeometryExporter::AppendUInt8(const u8 value) {
        int fileOffset = mFileSize;
        *(s8*)(mData + mFileSize + 0x20) = value;
        mFileSize++;
        return fileOffset;
    }

    int GeometryExporter::AppendVector3(const aiVector3D vector) {
        int offset = AppendFloat(vector.x);
        AppendFloat(vector.y);
        AppendFloat(vector.z);
        return offset;
    }

    int GeometryExporter::AppendFloat(const float value) {
        int fileOffset = mFileSize;
        *(float*)(mData + mFileSize + 0x20) = ByteSwap(value);
        mFileSize += 4;
        return fileOffset;
    }

    int GeometryExporter::AddPadding(int interval) {
        int remaining = interval - ((mFileSize + 0x20) % interval);
        if (remaining == interval)
            return mFileSize;

        mFileSize += remaining;
        return mFileSize;
    }

    template<typename ConfigType, typename AssimpType>
        ConfigType GetConfig(u32 index, std::vector<ConfigType> configList, AssimpType** assimpTypes, const char* (*getName)(AssimpType* value)) {
            // Get the config
            // Default to blank config
            ConfigType config; 

            // If there are enough configs for this texture, then use that index
            if (index < configList.size()) {
                config = configList[index];
            }

            // Check if names don't match
            AssimpType* texture = assimpTypes[index];
            if (config.mName != getName(texture)) {
                LogWarn("Writing texture '%d' and config at same index does not have same name. Expected '%s', got '%s'. Searching for matching texture names.", index, config.mName.c_str(), getName(texture));
                // Try to find correct file
                bool foundMatchingTextureName = false;
                for (size_t i = 0; i < configList.size(); i++) {
                    if (configList[i].mName == getName(texture)) {
                        foundMatchingTextureName = true;
                        config = configList[i];
                        break;
                    }
                }

                if (!foundMatchingTextureName) {
                    LogWarn("Failed to find texture config with matching texture name. Defaulting to using the same index for the config and texture.");
                }
            }

            return config;
        }

    const char* TextureGetName(aiTexture* texture) { return texture->mFilename.C_Str(); }
    TextureConfig GeometryExporter::GetTextureConfig(u32 textureIndex) {
        return GetConfig<TextureConfig, aiTexture>(textureIndex, mMapConfig.mTextureConfigs, mScene->mTextures, TextureGetName);
    }

    const char* MaterialGetName(aiMaterial* mat) { return mat->GetName().C_Str(); }
    MaterialConfig GeometryExporter::GetMaterialConfig(u32 materialIndex) {
        return GetConfig<MaterialConfig, aiMaterial>(materialIndex, mMapConfig.mMaterialConfigs, mScene->mMaterials, MaterialGetName);
    }
}
