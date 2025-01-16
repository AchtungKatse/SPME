#pragma once
#include "FileTypes/TPL.h"
#include "assimp/scene.h"
#include "FileTypes/LevelGeometry/InternalMapFile.h"

using namespace SPMEditor::LevelInternal;

namespace SPMEditor {
    class LevelGeometry
    {
        public:
            static aiScene* LoadFromBytes(const std::vector<u8>& data, TPL textures);

        private:
            struct Vertex
            {
                Vector3 position;
                Color color;
                Vector3 normal;
                Vector2 uv;
            };

            static Section FindSection(const std::string& name, int sectionTableOffset, int sectionCount);
            static void ReadSection(Section section, aiScene* scene);
            static void ReadMaterialNameTable(int tableOffset, aiScene* scene, int textureCount);
            static aiNode* ReadInfo(int offset, std::vector<aiMesh*>& meshes);
            static aiNode* ReadObject(int objectOffset, int& nextSibling, std::vector<aiMesh*>& meshes);
            static aiMesh* ReadMesh(int offset);

            static std::vector<std::string> ReadTextureNames(int offset);
            static VCDTable ReadVCDTable(int offset);
            static void ReadVertices(VCDTable vcd, int offset, VertexAttributes attributes, std::vector<Vertex>& vertices);


            static const u8* s_Data;
            static aiScene* s_CurrentScene;
            static int s_FirstMaterialAddress; // Required to calculate the material index for each mesh
    };
}
