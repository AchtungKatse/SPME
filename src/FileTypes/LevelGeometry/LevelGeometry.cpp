#include "FileTypes/LevelGeometry/LevelGeometry.h"
#include "FileTypes/LevelGeometry/MapStructures.h"
#include "Types/Types.h"
#include "core/Logging.h"
#include "assimp/anim.h"
#include "assimp/matrix4x4.h"
#include "core/filesystem.h"
#include "glm/ext/scalar_constants.hpp"

#define FPNG_NO_SSE 1
#include "assimp/material.h"
#include "assimp/quaternion.h"
#include "assimp/types.h"
#include "assimp/vector3.h"
#include "fpng.cpp" // stbi cant write to mempry

#include <numbers>
#include <string>

namespace SPMEditor {

    const u8* LevelGeometry::sData;
    aiScene* LevelGeometry::sCurrentScene;
    int LevelGeometry::s_FirstMaterialAddress; // Required to calculate the material index for each mesh
    LevelData* LevelGeometry::s_CurrentLevel;
    VCDTable LevelGeometry::s_VCDTable; 
    int objectCount = 0;

    aiScene* LevelGeometry::LoadFromBytes(const std::vector<u8>& fileData, TPL tpl, LevelData* level) {
        return LoadFromBytes(fileData.data(), fileData.size(), tpl, level);
    }

    aiScene* LevelGeometry::LoadFromBytes(const u8* data, u64 size, TPL tpl, LevelData* level) {
        sCurrentScene = new aiScene();
        s_CurrentLevel = level;

        // Start by loading textures since we have that right here
        sCurrentScene->mNumTextures = tpl.images.size();
        sCurrentScene->mTextures = new aiTexture*[tpl.images.size()];
        for (size_t i = 0; i < tpl.images.size(); i++) {
            TPL::Image image = tpl.images[i];
            aiTexture* texture = new aiTexture();

            std::vector<u8> pngPixels;
            fpng::fpng_encode_image_to_memory(image.pixels.data(), image.header.width, image.header.height, 4, pngPixels);
            texture->mWidth = pngPixels.size();
            texture->mHeight = 0;
            texture->pcData = new aiTexel[pngPixels.size()];
            memcpy(texture->achFormatHint, "png\x0", 4);

            u8* texturePixels = (u8*)texture->pcData;
            for (size_t p = 0; p < pngPixels.size(); p++)
            {
                texturePixels[p] = pngPixels[p];
            }

            sCurrentScene->mTextures[i] = texture;
        }

        // Load header
        FileHeader header = *(FileHeader*)data;
        ByteSwap4(&header, 4);

        // Grab the actual data from the file
        sData = data + 0x20; // skip the file header

        // Read the sectoins
        int sectionTableOffset = header.pointerEntryCount * 4 + header.pointerListStart;

        // Ok so some sections need to be read in a specific order because of assimp
        // Namely the material table because assimp references the material via an index
        // instead of a file pointer like the map data. who could have guessed that
        Section vcdSection = FindSection("vcd_table", sectionTableOffset, header.sectionCount);
        s_VCDTable = ReadVCDTable(vcdSection.fileOffset);

        Section materialSection = FindSection("material_name_table", sectionTableOffset, header.sectionCount);
        ReadMaterialNameTable(materialSection.fileOffset, tpl.images.size());

        Section infoSection = FindSection("information", sectionTableOffset, header.sectionCount);
        std::vector<aiMesh*> meshes;
        sCurrentScene->mRootNode = ReadInfoSection(infoSection.fileOffset, meshes);
        sCurrentScene->mNumMeshes = meshes.size();
        sCurrentScene->mMeshes = new aiMesh*[meshes.size()];
        for (uint i = 0; i < sCurrentScene->mNumMeshes; i++) {
            sCurrentScene->mMeshes[i] = meshes[i];
        }

        LogInfo("Scene has %d materials", sCurrentScene->mNumMaterials);

        for (int i = 0; i < header.sectionCount; i++) {
            Section section;
            section.fileOffset = ByteSwap(*(int*)(sData + sectionTableOffset + i * 8));
            int nameOffset = ByteSwap(*(int*)(sData + sectionTableOffset + i * 8 + 4));

            section.name = (char*)(sData + sectionTableOffset + nameOffset + 8 * header.sectionCount);

            LogInfo("Section 0x%x (%s) is at 0x%x", i, section.name.c_str(), section.fileOffset);
            ReadSection(section);
        }

        LogInfo("Total texture count: %u", sCurrentScene->mNumTextures);

        return sCurrentScene;
    }

    Section LevelGeometry::FindSection(const std::string& name, int sectionTableOffset, int sectionCount) {
        for (int i = 0; i < sectionCount; i++) {
            Section section;
            section.fileOffset = ByteSwap(*(int*)(sData + sectionTableOffset + i * 8));
            int nameOffset = ByteSwap(*(int*)(sData + sectionTableOffset + i * 8 + 4));

            section.name = (char*)(sData + sectionTableOffset + nameOffset + 8 * sectionCount);

            if (section.name == name)
                return section;
        }

        return Section();
    };

    void LevelGeometry::ReadSection(Section section) {
        // Skip over already read sections (in the readfrombytes function)
        switch (str2int(section.name.c_str())) {
            case str2int("material_name_table"): return;
            case str2int("vcd_table"): return;
        }

        LogInfo("---------- Reading Section %s (0x%x) ----------", section.name.c_str(), section.fileOffset);
        switch (str2int(section.name.c_str()))
        {
            default:
                LogWarn("----------- Section %s at offset 0x%x not implemented -----------", section.name.c_str(), section.fileOffset);
                return;
            case str2int("information"):
                    break;
            case str2int("texture_table"): {
                    std::vector<std::string> textureNames = ReadTextureNames(section.fileOffset);
                    // I swear to god if this ever happens
                    Assert(sCurrentScene->mNumTextures >= textureNames.size(), "Trying to read more texture names than there are textures! \n\tTexture Name Count: 0x%x\n\tScene Texture Count: %u", textureNames.size(), sCurrentScene->mNumTextures);
                    for (size_t i = 0; i < textureNames.size(); i++) {
                        const auto texture = sCurrentScene->mTextures[i];
                        sCurrentScene->mTextures[i]->mFilename = aiString(textureNames[i].c_str());
                        std::string path = "Images/" + textureNames[i];
                        filesystem_write_file(path.c_str(), (u8*)texture->pcData, (int)texture->mWidth);
                    }
                    break;
                }
            case str2int("light_table"):
                LogInfo("Map has 0x%d lights", *(int*)(sData + section.fileOffset));
                break;
            case str2int("fog_table"):
                ReadFogTable(section.fileOffset);
                break;
            case str2int("animation_table"):
                ReadAnimationTable(section.fileOffset);
                break;
            case str2int("curve_table"):
                ReadCurveTable(section.fileOffset);
                break;
        }
    }

    std::vector<std::string> LevelGeometry::ReadTextureNames(int offset) {
        int* textureTable = (int*)(sData + offset);
        int imageCount = ByteSwap(textureTable[0]);

        std::vector<std::string> names(imageCount);
        for (int i = 0; i < imageCount; i++) {
            names[i] = (char*)sData + ByteSwap(textureTable[i + 1]);
        }

        return names;
    }

    aiNode* LevelGeometry::ReadInfoSection(int offset, std::vector<aiMesh*>& meshes) {
        // Read the header
        int* headerPtr = (int*)(sData + offset);

        // Doing it this way instead of just casting the pointer to an info header
        // because char* is 8 bytes long while the offsets in the header are only 4 bytes long
        InfoSection info = {
            .version                = ByteSwap(headerPtr[0]),
            .objHeirarchyOffset     = ByteSwap(headerPtr[1]),
            .rootObjName            = ByteSwap(headerPtr[2]),
            .rootColliderName       = ByteSwap(headerPtr[3]),
            .timestamp              = ByteSwap(headerPtr[4]),
        };

        LogInfo("----- Info Header -----");
        LogInfo("File version:  %s", info.version.Get(sData));
        LogInfo("Root Obj:      %s", info.rootObjName.Get(sData));
        LogInfo("Root Trigger:  %s", info.rootColliderName.Get(sData));
        LogInfo("Timestamp:     %s", info.timestamp.Get(sData));

        LogInfo("----- Reading Objects -----");
        int siblingOffset;
        aiNode* rootObject = ReadObject(info.objHeirarchyOffset, siblingOffset, meshes);

        LogInfo("----- Object Count %d -----", objectCount);
        return rootObject;
    }

    aiNode* LevelGeometry::ReadObject(int objectOffset, int& nextSibling, std::vector<aiMesh*>& meshes, std::string indent) {
        objectCount++;
        // Read raw object data
        Object objectData = *(Object*)(sData + objectOffset);
        ByteSwap((int*)&objectData, sizeof(Object) / sizeof(int));
        Assert(objectData.padding == 0, "Object data is not padding. Expected 0, got 0x%x", objectData.padding);

        // Create new object
        aiNode* object = new aiNode();
        object->mName = aiString((char*)sData + objectData.name);

        // Copy the name
        std::string name = (char*)(sData + objectData.name);
        object->mName = new char[name.size()];
        object->mName.Set(name.c_str());

        std::string type = (char*)(sData + objectData.type);

        // Unfortunately the position is wonky so I need to do this
        Vector3 position = objectData.position;

        // Set the object transform
        object->mTransformation = aiMatrix4x4(objectData.scale, aiQuaternion(objectData.rotation.y / 180 * std::numbers::pi, objectData.rotation.z / 180 * std::numbers::pi, objectData.rotation.x / 180 * std::numbers::pi), position);

        // Try reading the meshes
        object->mMeshes = new u32[objectData.meshCount];
        object->mNumMeshes = objectData.meshCount;

        LogInfo("%sObject '%s'", indent.c_str(), name.c_str());
        LogInfo("\t%sType: %s, Offset: 0x%x, Mesh Count: %d", indent.c_str(), type.c_str(), objectOffset, objectData.meshCount);
        for (int i = 0; i < objectData.meshCount; i++) {
            // Read the material offset
            // Fun story, these two lines killed a days worth of bug fixing (~10 hours)
            // I forgot to add the i * 8 (read each mesh / material pair) so it was just reading 2 of the same meshes
            // and this made it so a lot of the objects were just missing. Man I hate programming sometimes
            int materialOffset = ByteSwap(*(int*)(sData + objectOffset + i * 8+ sizeof(Object)));
            int meshOffset = ByteSwap(*(int*)(sData + objectOffset + i * 8 + sizeof(Object) + 4));


            // Actually do the reading
            aiMesh* mesh = ReadMesh(meshOffset);
            mesh->mName = object->mName;
            u32 materialIndex = (materialOffset - s_FirstMaterialAddress) / sizeof(Material);

            mesh->mMaterialIndex = materialIndex;
            object->mMeshes[i] = meshes.size();

            // Add the mesh to the list
            // because the object only references the index of the mesh
            meshes.push_back(mesh);
        }


        // Read all the children

        // Read all the children
        // This works a bit weird because the sibling of the child object is another child object
        // so when there are no siblings remaining (nullptr) the loop stops since nextChild == 0
        int nextChild = objectData.child;
        std::vector<aiNode*> children;
        while (nextChild)
        {
            aiNode* childObject = ReadObject(nextChild, nextChild, meshes, indent + '\t');
            childObject->mParent = object;
            children.push_back(childObject);
        }

        object->mChildren = nullptr;
        if (children.size() > 0)
        {
            object->addChildren(children.size(), children.data());
        }

        nextSibling = objectData.nextSibling;

        return object;
    }

    aiMesh* LevelGeometry::ReadMesh(int offset) {
        MeshHeader header = *(MeshHeader*)(sData + offset);
        ByteSwap4(&header, 4);

        Assert(header.constant == 0x1000001, "Mesh header constant is not constant. Expected 0x1000001, got 0x%x", header.constant);

        aiMesh* mesh = new aiMesh();

        // Read each triangle entry
        VertexStrip::Header * stripHeaders = (VertexStrip::Header*)(sData + offset + sizeof(MeshHeader));
        std::vector<Vertex> vertices;
        std::vector<int> indices;
        for (int i = 0; i < header.entryCount; i++) {
            VertexStrip::Header vertexStrip = stripHeaders[i];
            ByteSwap((int*)&vertexStrip, 2);
            LogInfo("Reading vertex strip with attributes 0x%x", (u32)header.vertexAttributes);
            ReadVertices(s_VCDTable, vertexStrip.entryOffset, header.vertexAttributes, vertices, indices);
        }


        // Write to vertex buffer
        // Technically ignores vertex attributes but thats a future me problem
        mesh->mNumVertices = vertices.size();
        mesh->mVertices = new aiVector3t<float>[vertices.size()];

        mesh->mNumUVComponents[0] = 2;
        mesh->mTextureCoords[0] = new aiVector3D[ vertices.size() ];

        mesh->mNormals = new aiVector3t<float>[vertices.size()];

        if ((header.vertexAttributes & VERTEX_ATTRIBUTE_COLOR) != 0)  {
            mesh->mColors[0] = new aiColor4D[vertices.size()];
        }

        for (size_t i = 0; i < vertices.size(); i++) {
            mesh->mVertices[i] = vertices[i].position;
            mesh->mNormals[i] = vertices[i].normal;
            mesh->mNormals[i] = -mesh->mNormals[i].Normalize();
            mesh->mTextureCoords[0][i] = aiVector3D(vertices[i].uv.x, vertices[i].uv.y, 0);

            if ((header.vertexAttributes & VERTEX_ATTRIBUTE_COLOR) != 0) {
                mesh->mColors[0][i] = aiColor4D((float)vertices[i].color.r / 255, (float)vertices[i].color.g / 255, (float)vertices[i].color.b / 255, (float)vertices[i].color.a / 255);
            }
        }

        mesh->mNumFaces = indices.size() / 3;
        mesh->mFaces = new aiFace[ mesh->mNumFaces ];

        for (uint i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];

            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3];

            face.mIndices[0] = indices[i * 3 + 0];
            face.mIndices[1] = indices[i * 3 + 1];
            face.mIndices[2] = indices[i * 3 + 2];
        }

        return mesh;
    }

    void LevelGeometry::ReadVertices(VCDTable vcd, int offset, VertexAttributes attributes, std::vector<Vertex>& vertices, std::vector<int>& indices) {
        // Vertex UV and position scale factor has to do with float dequantization (pqx_lx instruction)
        // This happens at 0x8006d74c in the binary
        VertexStrip header = *(VertexStrip*)(sData + offset);
        header.vertexCount = ByteSwap(header.vertexCount);

        u16* vertexData = (u16*)(sData + offset + 3);
        for (uint i = 0; i < header.vertexCount; i++) {
            Vertex vertex;
            if (((u32)attributes & (u32)VertexAttributes::VERTEX_ATTRIBUTE_POSITION) != 0)
            {
                u16 vertexIndex = ByteSwap(*vertexData++);
                vec3<short> rawVertex = vcd.vertices.Get(sData)[vertexIndex];
                ByteSwap2(&rawVertex, 3);

                int scaleFactor = vcd.vertexScale;
                vertex.position = Vector3((float)rawVertex.x / scaleFactor, (float)rawVertex.y / scaleFactor, (float)rawVertex.z / scaleFactor);
            }
            if (((u32)attributes & (u32)VertexAttributes::VERTEX_ATTRIBUTE_NORMAL) != 0)
            {
                int index = ByteSwap(*vertexData++);
                vec3<s8> normal = vcd.normals.Get(sData)[index];
                vertex.normal.x = (float)(normal.x) / 0x40;
                vertex.normal.y = (float)(normal.y) / 0x40;
                vertex.normal.z = (float)(normal.z) / 0x40;
            }
            if (((u32)attributes & (u32)VertexAttributes::VERTEX_ATTRIBUTE_COLOR) != 0)
            {
                u16 colorIndex = ByteSwap(*vertexData);

                vertex.color = vcd.colors.Get(sData)[colorIndex];
                vertexData++;
            }
            if (((u32)attributes & (u32)VertexAttributes::VERTEX_ATTRIBUTE_UNK_1) != 0) {
                vertexData++;
            }
            if (((u32)attributes & (u32)VertexAttributes::VERTEX_ATTRIBUTE_UV) != 0) {
                int uvIndex = ByteSwap(*vertexData++);
                vec2<s16> rawUv = vcd.uvs.Get(sData)[uvIndex];
                ByteSwap2(&rawUv, 2);

                int uvScale = vcd.uvScale;
                Vector2 uv = Vector2((float)(short)rawUv.x / uvScale, 1.0f - (float)(short)rawUv.y / uvScale);
                vertex.uv = uv;
            }
            if (((u32)attributes & (u32)VertexAttributes::VERTEX_ATTRIBUTE_UNK_2) != 0) {
                vertexData++;
            }

            if (i >= 2) {
                int indexOffset = vertices.size() - 2;
                int a = indexOffset + 0;
                int b = indexOffset + 1;
                int c = indexOffset + 2;
                if (i % 2 != 0) {
                    // Front
                    indices.emplace_back(c);
                    indices.emplace_back(b);
                    indices.emplace_back(a);
                } else {
                    // Back
                    indices.emplace_back(a);
                    indices.emplace_back(b);
                    indices.emplace_back(c);
                }
            }

            vertices.push_back(vertex);
        }
    }

    VCDTable LevelGeometry::ReadVCDTable(int offset) {
        LogInfo("----- Reading VCD Table -----");
        VCDTable table = *(VCDTable*)(sData + offset);
        ByteSwap4(&table, sizeof(VCDTable) / 4);

        // NOTE:: Adding 4 to the addresses to skip count
        table.normals.address += 4;
        table.vertices.address += 4;
        table.uvs.address += 4;
        table.colors.address += 4;

        // NOTE:: Converting scales to their real value
        table.vertexScale = 1 << table.vertexScale;
        table.uvScale = 1 << table.uvScale;

        return table;
    }

    void LevelGeometry::ReadFogTable(int tableOffset) {
        u32 fogCount = ByteSwap(*(u32*)(sData + tableOffset));
        Assert(fogCount <= 1, "Unable to read fog table. Too many / few entries: %u", fogCount);
        if (fogCount <= 0)
            return;

        FogEntry fog = *(FogEntry*)(sData + tableOffset + 8);
        ByteSwap4(&fog, 2);
        s_CurrentLevel->fogSettings.emplace_back(fog);

        LogInfo("Fog near plane:        %f", fog.start); 
        LogInfo("Fog far plane:         %f", fog.end); 
        LogInfo("Fog Color:             (%x, %x, %x, %x)", fog.color.r, fog.color.g, fog.color.b, fog.color.a);
    }

    void LevelGeometry::ReadAnimationTable(int tableOffset) {
        uint animationCount = ByteSwap(*(uint*)(sData + tableOffset));
        uint* headers = (uint*)(sData + tableOffset + 4);

        sCurrentScene->mNumAnimations = animationCount;
        sCurrentScene->mAnimations = new aiAnimation*[animationCount];

        for (uint i = 0; i < animationCount; i++) {
            const AnimationHeader* header = (AnimationHeader*)(sData + ByteSwap(headers[i]));

            sCurrentScene->mAnimations[i] = new aiAnimation();
            aiAnimation* animation = sCurrentScene->mAnimations[i];
            animation->mName = (char*)sData + ByteSwap((int)header->nameOffset);
            animation->mDuration = ByteSwap(header->frameCount);
            animation->mTicksPerSecond = 25;

            LogTrace("Reading animation '%s' (0x%x) with %d frames.", animation->mName.C_Str(), ByteSwap(headers[i]), animation->mDuration);
            Assert(header->constant == 0, "Animation '%s' (Index: %d) header constant is not constant. Expected 0, got 0x%x", animation->mName.C_Str(), i, header->constant);

            if (header->transformAnimationOffset)
                ReadTransformAnimation(ByteSwap(header->transformAnimationOffset), animation);
            if (header->materialAnimationOffset)
                ReadTextureAnimation(ByteSwap(header->materialAnimationOffset));

            for (int pad = 0; pad < 4; pad++) {
                Assert(header->padding[pad] == 0, "Animation header padding has value 0x%x.", header->padding[pad]);
            }
        }
    }

    aiMatrix4x4 GetGlobalMatrix(aiNode* node) {
        aiVector3D nodeScale, nodePosition, nodeRotation;
        node->mTransformation.Decompose(nodeScale, nodeRotation, nodePosition);
        if (node->mParent) {
            return GetGlobalMatrix(node->mParent) * node->mTransformation;
        }

        return node->mTransformation;
    }

    void LevelGeometry::ReadTransformAnimation(int offset, aiAnimation* animation) {
        uint channelCount = ByteSwap(*(u32*)(sData + offset));
        animation->mNumChannels = channelCount;
        animation->mChannels = new aiNodeAnim*[channelCount];

        LogTrace("Reading transform animation (0x%x)", offset);
        for (uint i = 0; i < channelCount; i++) {
            int animPointer = ByteSwap(*(int*)(sData + offset + 4 + i * 4));
            TransformAnimation internalAnimation = *(TransformAnimation*)(sData + animPointer);
            ByteSwap4(&internalAnimation, sizeof(TransformAnimation) / 4);

            Assert(internalAnimation.keyframeCount > 0, "Animation '%s' has no keyframes", (char*)sData + internalAnimation.nameOffset);

            aiNodeAnim* channel = new aiNodeAnim();
            animation->mChannels[i] = channel;

            channel->mNodeName = (char*)sData + internalAnimation.nameOffset;
            channel->mNumPositionKeys   = internalAnimation.keyframeCount;
            channel->mNumScalingKeys    = internalAnimation.keyframeCount;
            channel->mNumRotationKeys   = internalAnimation.keyframeCount;

            channel->mPositionKeys = new aiVectorKey[internalAnimation.keyframeCount];
            channel->mScalingKeys  = new aiVectorKey[internalAnimation.keyframeCount];
            channel->mRotationKeys = new aiQuatKey[internalAnimation.keyframeCount];

            LogDebug("\tObject '%s' has %d frames (0x%x). Base Position: (%f, %f, %f), Base Rotation: (%f, %f, %f), Base Scale: (%f, %f, %f)", channel->mNodeName.C_Str(), internalAnimation.keyframeCount, animPointer,
                    internalAnimation.basePosition.x,
                    internalAnimation.basePosition.y,
                    internalAnimation.basePosition.z,
                    internalAnimation.baseRotation.x,
                    internalAnimation.baseRotation.y,
                    internalAnimation.baseRotation.z,
                    internalAnimation.baseScale.x,
                    internalAnimation.baseScale.y,
                    internalAnimation.baseScale.z
                    );
            TransformAnimation::Keyframe* keyframes = (TransformAnimation::Keyframe*)(sData + animPointer + sizeof(TransformAnimation));
            aiNode* node = sCurrentScene->mRootNode->FindNode(channel->mNodeName);
            Assert(node != nullptr, "Failed to find node '%s'", channel->mNodeName.C_Str());
            aiVector3D nodeScale, nodePosition, nodeRotation;
            node->mTransformation.Decompose(nodeScale, nodeRotation, nodePosition);
            LogDebug("\tPosition: (%f, %f, %f), Rotation: (%f, %f, %f), Scale: (%f, %f, %f)", nodePosition.x, nodePosition.y, nodePosition.z, nodeRotation.x, nodeRotation.y, nodeRotation.z, nodeScale.x, nodeScale.y, nodeScale.z);

            TransformAnimation::Keyframe startingKey = keyframes[0];
            ByteSwap4(&startingKey, sizeof(TransformAnimation::Keyframe) / 4);
            aiVector3D startingPosition(startingKey.position.x.start, startingKey.position.y.start, startingKey.position.z.start);

            for (u32 key = 0; key < internalAnimation.keyframeCount; key++) {
                TransformAnimation::Keyframe keyframe = keyframes[key];
                ByteSwap4(&keyframe, sizeof(TransformAnimation::Keyframe) / 4);

                constexpr double PI = glm::pi<double>();
                aiVector3D pos = aiVector3D(keyframe.position.x.start, keyframe.position.y.start, keyframe.position.z.start);// - aiVector3D(internalAnimation->basePosition.x, internalAnimation->basePosition.y, internalAnimation->basePosition.z);
                aiVector3D scale = aiVector3D(keyframe.scale.x.start, keyframe.scale.y.start, keyframe.scale.z.start);
                aiVector3D euler = aiVector3D(keyframe.rotation.x.start, keyframe.rotation.y.start, keyframe.rotation.z.start); 

                LogDebug("\t\tKeyframe %d. Time: %f, Local Position: (%f, %f, %f) Local Rotation: (%f, %f, %f), Local Scale: (%f, %f, %f)", key, keyframe.startFrame, 
                        pos.x, pos.y, pos.z,
                        euler.x, euler.y, euler.z,
                        scale.x, scale.y, scale.z);

                pos = pos - startingPosition + nodePosition;
                euler /= 180.0f;
                euler *= PI;

                LogDebug("\t\t\t\t\t\tPosition: (%f, %f, %f) Rotation: (%f, %f, %f), Scale: (%f, %f, %f)", 
                        pos.x, pos.y, pos.z,
                        euler.x, euler.y, euler.z,
                        scale.x, scale.y, scale.z);

                channel->mPositionKeys[key] = aiVectorKey(keyframe.startFrame, pos);
                channel->mScalingKeys[key]  = aiVectorKey(keyframe.startFrame, scale);
                channel->mRotationKeys[key] = aiQuatKey(keyframe.startFrame, aiQuaternion(euler.y, euler.z, euler.x));
            }

            Assert(animation->mChannels[i]->mNumPositionKeys == internalAnimation.keyframeCount, "Animatoin keyframe count does not match number of keyframes");
        }
    }

    void LevelGeometry::ReadTextureAnimation(int offset) { 
        uint objectCount = ByteSwap(*(u32*)(sData + offset));

        std::vector<MaterialAnimation> animations;
        for (uint i = 0; i < objectCount; i++) {
            int animationOffset = ByteSwap(*(u32*)(sData + offset + 4 + 4 * i));
            InternalMaterialAnimation* textureAnimation = (InternalMaterialAnimation*)(sData + animationOffset);
            InternalMaterialAnimation::Keyframe* keyframes = (InternalMaterialAnimation::Keyframe*)(sData + animationOffset + sizeof(InternalMaterialAnimation));

            MaterialAnimation animation;
            animation.animationName = ByteSwap(textureAnimation->materialNameOffset);

            animation.unknown[0] = textureAnimation->unknown[0];
            animation.unknown[1] = textureAnimation->unknown[1];
            animation.unknown[2] = textureAnimation->unknown[2];
            ByteSwap4(animation.unknown, 3);

            int keyframeCount = ByteSwap(textureAnimation->keyframeCount);
            animation.keyframes.reserve(keyframeCount);

            for (int frame = 0; frame < keyframeCount; frame++) {
                animation.keyframes.emplace_back(keyframes[frame]);
            }

            // Fix endianness of output frames
            if (keyframeCount > 0)
                ByteSwap4(animation.keyframes.data(), sizeof(InternalMaterialAnimation::Keyframe) / 4);
            animations.emplace_back(animation);

            LogInfo("Found Material Animation %s (0x%x)", animation.animationName.Get(sData), animationOffset);
            LogInfo("\tUnknown 0: %f", animation.unknown[0]);
            LogInfo("\tUnknown 1: %f", animation.unknown[1]);
            LogInfo("\tUnknown 2: %f", animation.unknown[2]);
            LogInfo("\tKeyframe count: %l", animation.keyframes.size());
        }
    }

    void LevelGeometry::ReadMaterialNameTable(int tableOffset, int textureCount) {
        u32 materialCount = ByteSwap(*(u32*)(sData + tableOffset));
        LogInfo("----- Reading Material Name Table -----");
        LogInfo("Material Count: 0x%x", materialCount);
        MaterialNameEntry* entries = (MaterialNameEntry*)(sData + tableOffset + 4);

        if (materialCount > 0)
            s_FirstMaterialAddress = ByteSwap(entries[0].materialOffset);

        sCurrentScene->mNumMaterials = materialCount;
        sCurrentScene->mMaterials = new aiMaterial*[materialCount];

        for (u32 i = 0; i < materialCount; i++) {
            // Read entry and material
            ByteSwap4(&entries[i], 2);
            Material material = *(Material*)(sData + entries[i].materialOffset);
            ByteSwap4(&material, 1); // Yay for funky data type
            ByteSwap4(((char*)&material + 0xc), 0x42); // Yay for funky data type

            // get the name
            aiString* name = new aiString((char*)(sData + entries[i].nameOffset));
            aiMaterial* mat = new aiMaterial();
            mat->AddProperty(name, AI_MATKEY_NAME);

            LogInfo("Material: '%s'", name->C_Str());
            LogInfo("\tUse Color: (%x, %x, %x, %x), Use Vertex Color: %x, Unk 1: %x, Use Transparency: %x, Use Texture: %x, Texture Info Ptr: 0x%x",
                    material.color.r, material.color.g, material.color.b, material.color.a, material.useVertexColor, material.unk_1, material.useTransparency, material.useTexture, material.textureInfoPtr);

            // So the texture index is calculated by the offset of the data pointer
            // The first thing in the map.dat file is the texture table which make calculating the texture index easy
            // 0x14 is the map.dat header
            // then textureCount * 0x10 is the each texture info header (name ptr, params, and size)
            // then the pointer that the material rerences
            if (material.textureInfoPtr != 0)
            {
                MapTexture::Info textureInfo = *(MapTexture::Info*)(sData + material.textureInfoPtr);
                ByteSwap4(&textureInfo, 2); // Just the first 2 ints
                MapTexture mapTexture = *(MapTexture*)(sData + textureInfo.dataOffset);
                mapTexture.nameOffset = ByteSwap(mapTexture.nameOffset);
                mapTexture.width = ByteSwap(mapTexture.width);
                mapTexture.height = ByteSwap(mapTexture.height);

                aiString* textureName = new aiString((char*)(sData + mapTexture.nameOffset));

                mat->AddProperty(textureName, AI_MATKEY_TEXTURE_DIFFUSE(0));
                LogInfo("\tReferences texture '%s'", textureName->C_Str());
            }

            aiColor4D color = aiColor4D((float)material.color.r / 255, (float)material.color.g / 255, (float)material.color.b / 255, (float)material.color.a / 255);
            mat->AddProperty(&color, 4, AI_MATKEY_BASE_COLOR);
            mat->AddProperty(&color, 4, AI_MATKEY_COLOR_DIFFUSE);

            sCurrentScene->mMaterials[i] = mat;
        }
    }

    void LevelGeometry::ReadCurveTable(int offset) {
        int curveCount = *(int*)(sData + offset);
        LogDebug("----------------------------------------------------- Curve table has 0x%x entries -----------------------------------------------------", curveCount);
        Assert(curveCount == 0, "Curve count is not zero: 0x%x", curveCount);
    }
}
