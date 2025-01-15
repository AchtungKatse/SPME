#include "LevelGeometry.h"
#include "InternalMapFile.h"
#include "Types/Types.h"
#include "assimp/material.h"
#include "assimp/quaternion.h"
#include "assimp/vector3.h"
#include <numbers>
#include <string>
#define FPNG_NO_SSE 1
#include "fpng.cpp" // wtf is this
#include "IO/FileWriter.h"

using namespace SPMEditor::LevelInternal;

namespace SPMEditor {

    // Vertex
    // This is here because its a datastructure used exclusively by myself in this file
    struct Vertex
    {
        Vector3 position;
        Color color;
        Vector3 normal;
        Vector2 uv;
    };

    //
    // Prototypes
    //
    vector<string> ReadTextureNames(u8* data, int offset);
    Section FindSection(string name, u8* data, int sectionTableOffset, int sectionCount);
    void ReadSection(Section section, u8* data, aiScene* scene);
    aiNode* ReadInfo(u8* data, int offset, vector<aiMesh*>& meshes);
    aiNode* ReadObject(u8* data, int objectOffset, int& nextSibling, vector<aiMesh*>& meshes);
    aiMesh* ReadMesh(u8* data, int offset);
    void ReadVertices(u8* data, VCDTable vcd, int offset, VertexAttributes attributes, vector<Vertex>& vertices);
    VCDTable ReadVCDTable(u8* data, int offset);
    void ReadMaterialNameTable(u8* data, int tableOffset, aiScene* scene, int textureCount);

    static int firstMaterialAddress = 0; // Required to calculate the material index for each mesh

    //
    // stroing
    //
    constexpr unsigned int str2int(const char* str, int h = 0)
    {
        return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
    }

    // 
    // Reading
    //
    aiScene* _scene;
    aiScene* LevelGeometry::LoadFromBytes(vector<u8> fileData, TPL tpl)
    {
        aiScene* scene = new aiScene();
        _scene = scene;
        // Start by loading textures since we have that right here
        scene->mNumTextures = tpl.images.size();
        scene->mTextures = new aiTexture*[tpl.images.size()];
        for (int i = 0; i < tpl.images.size(); i++) {
            TPL::Image image = tpl.images[i];
            aiTexture* texture = new aiTexture();

            vector<u8> pngPixels;
            fpng::fpng_encode_image_to_memory(image.pixels.data(), image.header.width, image.header.height, 4, pngPixels);
            texture->mWidth = pngPixels.size();
            texture->mHeight = 0;
            texture->pcData = new aiTexel[pngPixels.size()];
            memcpy(texture->achFormatHint, "png", 3);

            u8* texturePixels = (u8*)texture->pcData;
            for (int p = 0; p < pngPixels.size(); p++)
            {
                texturePixels[p] = pngPixels[p];
            }

            scene->mTextures[i] = texture;
        }

        // Load header
        FileHeader header = *(FileHeader*)fileData.data();
        ByteSwap4(&header, 4);

        // Grab the actual data from the file
        u8* data = fileData.data() + 0x20; // skip the file header

        // Read the sectoins
        int sectionTableOffset = header.fatEntryCount * 4 + header.fatPointer;

        // Ok so some sections need to be read in a specific order because of assimp
        // Namely the material table because assimp references the material via an index
        // instead of a file pointer like the map data. who could have guessed that
        Section materialSection = FindSection("material_name_table", data, sectionTableOffset, header.sectionCount);
        ReadMaterialNameTable(data, materialSection.fileOffset, scene, tpl.images.size());
        cout << "Scene has " << scene->mNumMaterials << " materials: " << scene->mMaterials[0]->GetName().C_Str() << endl;


        for (int i = 0; i < header.sectionCount; i++) {
            Section section;
            section.fileOffset = ByteSwap(*(int*)(data + sectionTableOffset + i * 8));
            int nameOffset = ByteSwap(*(int*)(data + sectionTableOffset + i * 8 + 4));

            section.name = (char*)(data + sectionTableOffset + nameOffset + 8 * header.sectionCount);

            cout << "Section 0x" << hex << i << " (" << section.name << ") is at 0x" << section.fileOffset << endl;
            ReadSection(section, data, scene);
        }

        cout << "Total texture count: " << dec << scene->mNumTextures << endl;

        return scene;
    }

    Section FindSection(string name, u8* data, int sectionTableOffset, int sectionCount)
    {
        for (int i = 0; i < sectionCount; i++) {
            Section section;
            section.fileOffset = ByteSwap(*(int*)(data + sectionTableOffset + i * 8));
            int nameOffset = ByteSwap(*(int*)(data + sectionTableOffset + i * 8 + 4));

            section.name = (char*)(data + sectionTableOffset + nameOffset + 8 * sectionCount);

            if (section.name == name)
            {
                return section;
            }
        }

        return Section();
    };

    void ReadSection(Section section, u8* data, aiScene* scene)
    {
        // Skip over already read sections (in the readfrombytes function)
        switch (str2int(section.name.c_str()))
        {
            case str2int("material_name_table"): return;
        }

        cout << "Trying to read section " << section.name << " at offset 0x " << section.fileOffset << endl;
        switch (str2int(section.name.c_str()))
        {
            default:
                {
                    cout << "Section " << section.name << " at offset 0x " << section.fileOffset << " not implemented" << endl;
                }
                return;
            case str2int("information"):
                {
                    vector<aiMesh*> meshes;
                    scene->mRootNode = ReadInfo(data, section.fileOffset, meshes);
                    scene->mNumMeshes = meshes.size();
                    scene->mMeshes = new aiMesh*[meshes.size()];
                    for (int i = 0; i < scene->mNumMeshes; i++) {
                        scene->mMeshes[i] = meshes[i];
                    }
                    break;
                }
            case str2int("texture_table"):
                {
                    vector<string> textureNames = ReadTextureNames(data, section.fileOffset);
                    if (scene->mNumTextures < textureNames.size())
                    {
                        // I swear to god if this ever happens
                        cout << "Trying to read more texture names than there are textures! \n\tTexture Name Count: 0x" << textureNames.size() << "\n\tScene Texture Count" << scene->mNumTextures << endl;
                        break;
                    }
                    for (int i = 0; i < textureNames.size(); i++) {
                        const auto texture = scene->mTextures[i];
                        scene->mTextures[i]->mFilename = aiString(textureNames[i].c_str());
                        FileWriter::WriteFile("Images/" + textureNames[i], (u8*)texture->pcData, (int)texture->mWidth);
                    }
                    break;
                }
            case str2int("light_table"):
                {
                    cout << endl << endl << endl;
                    cout << "Map has 0x" << hex << *(int*)(data + section.fileOffset) << " lights" << endl;
                    cout << endl << endl << endl;
                    break;
                }
        }
    }

    vector<string> ReadTextureNames(u8* data, int offset)
    {
        int* textureTable = (int*)(data + offset);
        int imageCount = ByteSwap(textureTable[0]);

        vector<string> names(imageCount);
        for (int i = 0; i < imageCount; i++) {
            names[i] = (char*)data + ByteSwap(textureTable[i + 1]);
        }

        return names;
    }

    u32 numObjects = 0;
    aiNode* ReadInfo(u8* data, int offset, vector<aiMesh*>& meshes)
    {
        // Read the header
        int* headerPtr = (int*)(data + offset);

        // Doing it this way instead of just casting the pointer to an info header
        // because char* is 8 bytes long while the offsets in the header are only 4 bytes long
        InfoHeader info;
        info.version = (char*)(long)ByteSwap(headerPtr[0]);
        info.objHeirarchyOffset = ByteSwap(headerPtr[1]);
        info.rootObjName = (char*)(long)ByteSwap(headerPtr[2]);
        info.rootTriggerName = (char*)(long)ByteSwap(headerPtr[3]);
        info.timestamp = (char*)(long)ByteSwap(headerPtr[4]);

        // Add the data ptr to each pointer in the info header
        // because the offsets are relative to the start of the file (data)
        info.version += (long)data;
        info.timestamp += (long)data;
        info.rootObjName += (long)data;
        info.rootTriggerName += (long)data;

        cout << "File version: " << info.version << endl;
        cout << "Root Obj: " << info.rootObjName << endl;
        cout << "Root Trigger: " << info.rootTriggerName << endl;
        cout << "Timestamp: " << info.timestamp << endl;

        int siblingOffset;
        aiNode* rootObject = ReadObject(data, info.objHeirarchyOffset, siblingOffset, meshes);

        cout << "Total object count: 0x " << hex << numObjects << endl;
        return rootObject;
    }

    aiNode* ReadObject(u8* data, int objectOffset, int& nextSibling, vector<aiMesh*>& meshes) 
    {
        // Read raw object data
        RawObject objectData = *(RawObject*)(data + objectOffset);
        ByteSwap((int*)&objectData, sizeof(RawObject) / sizeof(int));

        // Create new object
        aiNode* object = new aiNode();

        // Copy the name
        string name = (char*)(data + objectData.name);
        object->mName = new char[name.size()];
        object->mName.Set(name.c_str());

        string type = (char*)(data + objectData.type);

        // Unfortunately the position is wonky so I need to do this
        Vector3 position = objectData.position;

        // Set the object transform
        object->mTransformation = aiMatrix4x4(objectData.scale, aiQuaternion(objectData.rotation.y / 180 * numbers::pi, objectData.rotation.z / 180 * numbers::pi, objectData.rotation.x / 180 * numbers::pi), position);

        // Try reading the meshes
        object->mMeshes = new u32[objectData.meshCount];
        object->mNumMeshes = objectData.meshCount;
        numObjects += max(1, objectData.meshCount);

        LogInfo("Object: {} found at 0x{:x} with {} meshes", name, objectData.meshCount, objectOffset);
        for (int i = 0; i < objectData.meshCount; i++) {
            // Read the material offset
            // Fun story, these two lines killed a days worth of bug fixing (~10 hours)
            // I forgot to add the i * 8 (read each mesh / material pair) so it was just reading 2 of the same meshes
            // and this made it so a lot of the objects were just missing. Man I hate programming sometimes
            int materialOffset = ByteSwap(*(int*)(data + objectOffset + i * 8+ sizeof(RawObject)));
            int meshOffset = ByteSwap(*(int*)(data + objectOffset + i * 8 + sizeof(RawObject) + 4));


            // Actually do the reading
            cout << "\tMesh: 0x " << hex << meshOffset << endl;
            aiMesh* mesh = ReadMesh(data, meshOffset);
            u32 materialIndex = (materialOffset - firstMaterialAddress) / sizeof(Material);

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
        vector<aiNode*> children;
        while (nextChild)
        {
            aiNode* childObject = ReadObject(data, nextChild, nextChild, meshes);
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

    aiMesh* ReadMesh(u8* data, int offset)
    {
        // cout << "\tHeader at 0x: " << offset << endl;
        MeshHeader header = *(MeshHeader*)(data + offset);
        ByteSwap4(&header, 4);

        aiMesh* mesh = new aiMesh();
        VCDTable vcd = ReadVCDTable(data, header.VCDOffset);

        // Read each triangle entry
        VertexStrip::Header * stripHeaders = (VertexStrip::Header*)(data + offset + sizeof(MeshHeader));
        vector<Vertex> vertices;
        vector<int> indices;
        int indexOffset = 0;
        for (int i = 0; i < header.entryCount; i++) {
            VertexStrip::Header vertexStrip = stripHeaders[i];
            ByteSwap((int*)&vertexStrip, 2);
            ReadVertices(data, vcd, vertexStrip.entryOffset, header.vertexAttributes, vertices);

            for (int v = indexOffset; v < vertices.size(); v++)
            {
                // Dont add triangles to non-existant vertices
                if (v >= vertices.size() - 2)
                    continue;

                indices.push_back(v + 0);
                indices.push_back(v + 1);
                indices.push_back(v + 2);
            }

            indexOffset = vertices.size();
        }


        // Write to vertex buffer
        // Technically ignores vertex attributes but thats a future me problem
        mesh->mNumVertices = vertices.size();
        mesh->mVertices = new aiVector3t<float>[vertices.size()];

        mesh->mNumUVComponents[ 0 ] = 2; // Fuck this line in particular. I spent like 5 hours debugging it because the reference I used was bad
        mesh->mTextureCoords[0] = new aiVector3D[ vertices.size() ];

        mesh->mNormals = new aiVector3t<float>[vertices.size()];

        if (((u32)header.vertexAttributes & (u32)VertexAttributes::Color) != 1) 
            mesh->mColors[0] = new aiColor4D[vertices.size()];

        for (int i = 0; i < vertices.size(); i++) {
            mesh->mVertices[i] = vertices[i].position;
            // mesh->mNormals[i] = vertices[i].normal;
            mesh->mTextureCoords[0][i] = aiVector3D(vertices[i].uv.x, vertices[i].uv.y, 0);

            if (((u32)header.vertexAttributes & (u32)VertexAttributes::Color) != 1) 
                mesh->mColors[0][i] = aiColor4D((float)vertices[i].color.r / 255, (float)vertices[i].color.g / 255, (float)vertices[i].color.b / 255, (float)vertices[i].color.a / 255);
        }

        mesh->mNumFaces = indices.size() / 3;
        mesh->mFaces = new aiFace[ mesh->mNumFaces ];

        for (int i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];

            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3];

            face.mIndices[0] = indices[i * 3 + 0];
            face.mIndices[1] = indices[i * 3 + 1];
            face.mIndices[2] = indices[i * 3 + 2];
        }

        return mesh;
    }

    int maxScream = 0;
    void ReadVertices(u8* data, VCDTable vcd, int offset, VertexAttributes attributes, vector<Vertex>& vertices)
    {
        VertexStrip header = *(VertexStrip*)(data + offset);
        header.vertexCount = ByteSwap(header.vertexCount);

        u16* vertexData = (u16*)(data + offset + 3);
        for (int i = 0; i < header.vertexCount; i++) {
            Vertex vertex;
            if (((u32)attributes & (u32)VertexAttributes::Position) != 0)
            {
                vec3<short> rawVertex = vcd.vertices[ByteSwap(*vertexData++)];
                ByteSwap2(&rawVertex, 3);

                int scaleFactor = 64;
                vertex.position = Vector3((float)rawVertex.x / scaleFactor, (float)rawVertex.y / scaleFactor, (float)rawVertex.z / scaleFactor);
            }
            if (((u32)attributes & (u32)VertexAttributes::Unk_1) != 0)
            {
                int index = ByteSwap(*vertexData++);
                if (index > maxScream)
                {
                    cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAA: " << hex << index << endl << endl;
                    maxScream = index;
                }
            }
            if (((u32)attributes & (u32)VertexAttributes::Color) != 0)
            {
                u16 colorIndex = ByteSwap(*vertexData);

                u8 r = *((u8*)(vcd.colors) + colorIndex * sizeof(Color) + 0);
                u8 g = *((u8*)(vcd.colors) + colorIndex * sizeof(Color) + 1);
                u8 b = *((u8*)(vcd.colors) + colorIndex * sizeof(Color) + 2);
                u8 a = *((u8*)(vcd.colors) + colorIndex * sizeof(Color) + 3);

                Color rawColor(r,g,b,a);

                vertex.color = rawColor;
                vertexData++;
            }
            if (((u32)attributes & (u32)VertexAttributes::Unk_2) != 0)
            {
                vertexData++;
            }
            if (((u32)attributes & (u32)VertexAttributes::UV) != 0)
            {
                int uvIndex = ByteSwap(*vertexData++);
                vec2<u16> rawUv = vcd.uvs[uvIndex];
                ByteSwap2(&rawUv, 2);

                int uvScale = 0x100;
                Vector2 uv = Vector2((float)(short)rawUv.x / uvScale, 1.0f - (float)(short)rawUv.y / uvScale);
                vertex.uv = uv;
            }

            vertices.push_back(vertex);
        }
    }

    VCDTable ReadVCDTable(u8* data, int offset)
    {
        VCDTable table;

        // This function is funky
        // I doubt it is 100% accurate
        // It looks like the VCDTable has a somewhat dynamic struct
        // where some sections have a count followed by a number of pointers
        int* offsetPtrs = (int*)(data + offset);
        int vertexOffset = ByteSwap(offsetPtrs[0]);
        int lightsOffset = ByteSwap(offsetPtrs[1]);
        int colorsOffset = ByteSwap(offsetPtrs[3]);
        int uvOffset = ByteSwap(offsetPtrs[6]);

        // cout << "VCD Table Offset: 0x " << offset << endl;
        // cout << "UV Table: 0x " << uvOffset << endl;
        table.vertexCount = ByteSwap(*(int*)(data + vertexOffset));
        table.lightColorCount = ByteSwap(*(int*)(data + lightsOffset));
        table.colorCount = ByteSwap(*(int*)(data + colorsOffset));
        table.uvCount = ByteSwap(*(int*)(data + uvOffset));

        table.vertices = (vec3<short>*)(data + vertexOffset + 4);
        table.lightColors = (Color*)(data + lightsOffset + 4);
        table.colors = (Color*)(data + colorsOffset + 4);
        table.uvs = (vec2<u16>*)(data + uvOffset + 4);

        return table;
    }

    void ReadMaterialNameTable(u8* data, int tableOffset, aiScene* scene, int textureCount)
    {
        u32 materialCount = ByteSwap(*(u32*)(data + tableOffset));
        cout << "Found " << dec << materialCount << " materials" << endl;
        MaterialNameEntry* entries = (MaterialNameEntry*)(data + tableOffset + 4);

        if (materialCount > 0)
            firstMaterialAddress = ByteSwap(entries[0].materialOffset);

        scene->mNumMaterials = materialCount;
        scene->mMaterials = new aiMaterial*[materialCount];

        for (int i = 0; i < materialCount; i++) {
            // Read entry and material
            ByteSwap4(&entries[i], 2);
            Material material = *(Material*)(data + entries[i].materialOffset);
            ByteSwap4(&material, 1); // Yay for funky data type
            ByteSwap4(((char*)&material + 0xc), 0x42); // Yay for funky data type

            // get the name
            aiString* name = new aiString((char*)(data + entries[i].nameOffset));

            aiMaterial* mat = new aiMaterial();
            mat->AddProperty(name, AI_MATKEY_NAME);

            // So the texture index is calculated by the offset of the data pointer
            // The first thing in the map.dat file is the texture table which make calculating the texture index easy
            // 0x14 is the map.dat header
            // then textureCount * 0x10 is the each texture info header (name ptr, params, and size)
            // then the pointer that the material rerences
            cout << "Material " << name->C_Str() << " (0x " << hex << entries[i].materialOffset << " ) "<< endl;
            if (material.textureInfoPtr != 0)
            {
                MapTexture::Info textureInfo = *(MapTexture::Info*)(data + material.textureInfoPtr);
                ByteSwap4(&textureInfo, 2); // Just the first 2 ints
                MapTexture mapTexture = *(MapTexture*)(data + textureInfo.dataOffset);
                mapTexture.nameOffset = ByteSwap(mapTexture.nameOffset);
                mapTexture.width = ByteSwap(mapTexture.width);
                mapTexture.height = ByteSwap(mapTexture.height);

                aiString* textureName = new aiString((char*)(data + mapTexture.nameOffset));

                mat->AddProperty(textureName, AI_MATKEY_TEXTURE_DIFFUSE(0));

                cout << dec << "\tReferences texture '" << textureName->C_Str() << "'" << endl;
            }

            cout << "\tColor: " << hex << *(int*)&material.color << endl;
            aiColor4D color = aiColor4D((float)material.color.r / 255, (float)material.color.g / 255, (float)material.color.b / 255, (float)material.color.a / 255);
            mat->AddProperty(&color, 4, AI_MATKEY_BASE_COLOR);
            mat->AddProperty(&color, 4, AI_MATKEY_COLOR_DIFFUSE);

            scene->mMaterials[i] = mat;
        }
    }
}
