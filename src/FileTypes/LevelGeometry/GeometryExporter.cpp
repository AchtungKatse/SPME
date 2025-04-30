#include "FileTypes/LevelGeometry/GeometryExporter.h"
#include "FileTypes/LevelGeometry/InternalMapFile.h"
#include "FileTypes/TPL.h"
#include "Types/Types.h"
#include "Utility/Logging.h"
#include "assimp/material.h"
#include "assimp/types.h"
#include "assimp/vector3.h"
#include <algorithm>
#include <fstream>
#include <ios>
#include <numbers>
#include <string>
#include "stb_image.h"

using namespace SPMEditor::LevelInternal;
namespace SPMEditor {
    GeometryExporter* GeometryExporter::Create() { 
        return new GeometryExporter();
    }

    GeometryExporter::GeometryExporter() : mVCDTable({}) {
        memset(&mVCDTable, 0, sizeof(RawVCDTable));
    }
    GeometryExporter::~GeometryExporter() {
        delete mData;
        delete mTextBuffer;
    };

    GeometryExporter::Section::Section(const char* name, int offset) : name(name), offset(offset) { }

    void GeometryExporter::Write(const aiScene* scene, const std::string& path) {
        mScene = scene;

        // For whatever reason, blender's gltf export results in all images having the name "Image-Image" which seems to confuse SPM
        // HACK: the fix for this is just going to be to append image indices to their names
        // NOTE: This doesn't seem to actually affect the textures used since materials reference it via an ImageInfo*
        for (size_t i = 0; i < scene->mNumTextures; i++) {
            scene->mTextures[i]->mFilename.Append("_index_");
            scene->mTextures[i]->mFilename.Append(std::to_string(i).c_str());
        }

        // Write the scene images as a TPL
        WriteTPL();

        std::ofstream output(path);

        mFileSize = 0;
        mTextBufferSize = 0;

        constexpr int DataBufferSize = 1024 * 1024 * 50; // 50MB
        constexpr int TextBufferSize = 1024 * 1024 * 10; // 10MB
        mData = new char[DataBufferSize]; // Overly large buffer
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
        output.write(mData, mFileSize);
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
                int channels = 0;
                Color* colors = (Color*)stbi_load_from_memory((u8*)texture->pcData, texture->mWidth, (int*)&mapTexture.width, (int*)&mapTexture.height, &channels, 0);

                // Determin transparency type
                if (channels == 4) {
                    for (int i = 0; i < mapTexture.width * mapTexture.height; i++) {
                        // TODO: Research other transparency modes
                        if (colors[i].a != 0xFF) {
                            mapTexture.transparency = MapTexture::TransparencyType::Clip;
                        }
                    }
                }

            }
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
            int address = AppendPointer(0x14 + i * sizeof(MapTexture));
            AppendInt32(0); // padding
            // TODO: The following u8's are wrap mode,
            // This defaults them to repeat but should be user configurable
            AppendInt8(1);
            AppendInt8(1);
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

        char nameBuffer[nameBufferSize];

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
        mVCDTable.vertexOffset = AppendInt32(mVertexTable.size());
        mFileSize += mVertexTable.size() * 6;
        u32 _vertexScale = 1 << vertexScale;
        for (const auto pair : mVertexTable) {
            const auto v = pair.first;
            int vertexIndex = pair.second;
            *(u16*)(mData + mVCDTable.vertexOffset + vertexIndex * 0x6 + 0x24) = ByteSwap((s16)(v.x * _vertexScale));
            *(u16*)(mData + mVCDTable.vertexOffset + vertexIndex * 0x6 + 0x26) = ByteSwap((s16)(v.y * _vertexScale));
            *(u16*)(mData + mVCDTable.vertexOffset + vertexIndex * 0x6 + 0x28) = ByteSwap((s16)(v.z * _vertexScale));
        }

        AddPadding(0x20);
        mVCDTable.colorOffset = AppendInt32(mColorTable.size());
        mFileSize += mColorTable.size() * 4;
        for (const auto pair : mColorTable) {
            const auto color = pair.first;
            int index = pair.second;
            *(u8*)(mData + mVCDTable.colorOffset + index * 0x4 + 0x24) = ByteSwap((u8)(color.r * 255));
            *(u8*)(mData + mVCDTable.colorOffset + index * 0x4 + 0x25) = ByteSwap((u8)(color.r * 255));
            *(u8*)(mData + mVCDTable.colorOffset + index * 0x4 + 0x26) = ByteSwap((u8)(color.r * 255));
            *(u8*)(mData + mVCDTable.colorOffset + index * 0x4 + 0x27) = ByteSwap((u8)(color.r * 255));
        }

        AddPadding(0x20);
        mVCDTable.uvOffset = AppendInt32(mUvTable.size());
        mFileSize += mUvTable.size() * 4;
        u32 _uvScale = 1 << uvScale;
        for (const auto pair : mUvTable) {
            const auto uv = pair.first;
            int index = pair.second;
            *(u16*)(mData + mVCDTable.uvOffset + index * 0x4 + 0x24) = ByteSwap((s16)(uv.x * _uvScale));
            *(u16*)(mData + mVCDTable.uvOffset + index * 0x4 + 0x26) = ByteSwap((s16)((1.0f - uv.y) * _uvScale)); // 1.0f - uv.y fixes textures from being upside down
        }

        AddPadding(0x20);
        mVCDTable.normalOffset = AppendInt32(mNormalTable.size());
        LogInfo("Writing %u normals.", mNormalTable.size());
        for (const auto pair : mNormalTable) {
            const auto normal = pair.first;
            int index = pair.second;
            *(u8*)(mData + mVCDTable.normalOffset + index * 0x4 + 0x24) = ByteSwap((u8)(normal.x * 64));
            *(u8*)(mData + mVCDTable.normalOffset + index * 0x4 + 0x24) = ByteSwap((u8)(normal.y * 64));
            *(u8*)(mData + mVCDTable.normalOffset + index * 0x4 + 0x24) = ByteSwap((u8)(normal.z * 64));
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
                if (mesh->HasVertexColors(0))
                    if (!mColorTable.contains(mesh->mColors[0][v]))
                        mColorTable.emplace(mesh->mColors[0][v], mColorTable.size());

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

            aiVector3D materialColor(1);
            float opacity = 0;
            if (AI_SUCCESS != material->Get(AI_MATKEY_BASE_COLOR, materialColor)) LogWarn("Failed to get diffuse color from material '%s'", material->GetName().C_Str());
            if (AI_SUCCESS != material->Get(AI_MATKEY_OPACITY, opacity)) LogWarn("Failed to get opacity from material '%s'", material->GetName().C_Str());

            AppendUInt8((u8)(materialColor.x * 255));
            AppendUInt8((u8)(materialColor.y * 255));
            AppendUInt8((u8)(materialColor.z * 255));
            /*AppendUInt8((u8)((1 - opacity) * 255));*/
            AppendUInt8(255);

            // TODO: Allow user to configure materials
            u8 useVertexColors = 1;
            u8 unk_1 = 1;
            // TODO: Transparency will be disabled until configuration is implemented because it results in non-transparent objects being culled.
            u8 useTransparency = 1;
            u8 useTexture = material->GetTextureCount(aiTextureType_DIFFUSE) > 0;
            /*useTexture = false;*/
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
                    LogTrace("\tUsing texture index: %d", imageIndex);
                    textureInfoPtr += imageIndex * sizeof(MapTexture::Info);

                }
                else {
                    LogWarn("Texture Name To Index table does not have texture '%s'. Using first texture.", path->C_Str());
                    textureInfoPtr += sizeof(MapTexture) * (i % mScene->mNumTextures);
                }
                LogInfo("Material texture path '%s' has index of %d and offset of %p", path->C_Str(), mTextureNameToIndex[path->C_Str()], textureInfoPtr);
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
        VertexAttributes attr = VertexAttributes::Position;
        if (mesh->HasNormals()) {
            attr = (VertexAttributes)((int)attr | (int)VertexAttributes::Normal);
            vertexSize += 2;
        }
        if (mesh->HasVertexColors(0)) {
            attr = (VertexAttributes)((int)attr | (int)VertexAttributes::Color);
            vertexSize += 2;
        }
        if (mesh->HasTextureCoords(0)) {
            attr = (VertexAttributes)((int)attr | (int)VertexAttributes::UV); 
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
        int nextSiblingOffset = AppendInt32(0);

        if (previousSibling)
            previousSibling = AppendPointer(previousSibling);
        else previousSibling = AppendInt32(0);

        aiVector3D position, rotation, scale;
        node->mTransformation.Decompose(scale, rotation, position);
        AppendVector3(scale);
        AppendVector3(rotation * 180.0f / (float)std::numbers::pi);
        AppendVector3(position);

        aiVector3D boundsMin(0);
        aiVector3D boundsMax(1);
        AppendVector3(boundsMin);
        AppendVector3(boundsMax);

        AppendInt32(0);
        if (parentOffset == 0) { // Is root
            int subdataOffset = AppendInt32(0); // subdataptr
        } else {
            int subdataOffset = AppendPointer(0); // subdataptr
            mObjectSubdatas.emplace_back(ObjectSubData{RawObject::SubData(), subdataOffset});
            mPointerList.emplace_back(subdataOffset);

        }

        LogTrace("Wrting %u meshes", node->mNumMeshes);
        AppendInt32(node->mNumMeshes);

        // Add filler for no reason, might try to remove this later
        if (node->mNumMeshes <= 0) {
            AppendInt32(0);
            AppendInt32(0);
        }

        for (size_t i = 0; i < node->mNumMeshes; i++) {
            const aiMesh* mesh = mScene->mMeshes[node->mMeshes[i]];
            LogInfo("Mesh has material index %u ('%s')", mesh->mMaterialIndex, mScene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str());
            int materialOffset = AppendPointer(mMaterialTable[mesh->mMaterialIndex].materialOffset);
            int meshOffset = AppendPointer(mMeshTable[node->mMeshes[i]]);
        }

        // Write the children
        // TODO: remove variable length array
        RawObject* children[node->mNumChildren];
        int childrenOffsets[node->mNumChildren];

        u32 previousChild = 0;
        for (size_t i = 0; i < node->mNumChildren; i++) {
            int childOffset = WriteObject(node->mChildren[i], selfAddress, previousChild);
            previousSibling = childOffset;
            children[i] = (RawObject*)(mData + 0x20 + childOffset);
            childrenOffsets[i] = childOffset;
        }

        // Write children siblings
        for (size_t i = 0; i < node->mNumChildren; i++) {
            // Write next
            if (i < node->mNumChildren - 1) {
                children[i]->nextSibling = ByteSwap(childrenOffsets[i + 1]);
                AddPointer(childrenOffsets[i] + offsetof(RawObject, nextSibling));
            }
            // Write previous
            if (i > 0) {
                children[i]->previousSibling = ByteSwap(childrenOffsets[i - 1]);
                AddPointer(childrenOffsets[i] + offsetof(RawObject, previousSibling));
            }
        }

        if (node->mNumChildren > 0) {
            *(u32*)(mData + 0x20 + childPtrOffset) = ByteSwap(childrenOffsets[0]);
            AddPointer(childPtrOffset);
        }

        return selfAddress;
    }

    int GeometryExporter::GetObjectSize(const aiNode* node) {
        return sizeof(RawObject) + std::max(node->mNumMeshes, 1u) * 8;
    }

    void GeometryExporter::WriteVCDTable() {
        mVCDAddress = AppendPointer(mVCDTable.vertexOffset);
        AppendPointer(mVCDTable.normalOffset);
        AppendInt32(0);
        AppendPointer(mVCDTable.colorOffset);
        AppendInt32(mVCDTable.unknown_2);
        AppendInt32(mVCDTable.unknown_3);
        AppendPointer(mVCDTable.uvOffset);
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

    void GeometryExporter::WriteTPL() {
        TPLImageCreateInfo imageCreateInfos[mScene->mNumTextures];
        for (size_t i = 0; i < mScene->mNumTextures; i++) {
            imageCreateInfos[i].mFormat = TPLImageFormat::RGBA32;
            imageCreateInfos[i].mTexture = mScene->mTextures[i];
        }

        TPLCreateInfo createInfo = {
            .mImageCreateInfos = imageCreateInfos,
            .mImageCreateInfoCount = mScene->mNumTextures,
        };

        TPL tpl = TPL::CreateTPL(createInfo);
        tpl.Write("test.tpl");
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
        LogTrace("Writing text '%s' at %d (string offset: %d)", text, fileOffset, offset);

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
}
