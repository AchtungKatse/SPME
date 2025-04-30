#pragma once
#include "FileTypes/LevelData.h"
#include "FileTypes/TPL.h"
#include "assimp/scene.h"
#include "FileTypes/LevelGeometry/InternalMapFile.h"

using namespace SPMEditor::LevelInternal;

namespace SPMEditor {
    class LevelGeometry
    {
        public:
            static aiScene* LoadFromBytes(const std::vector<u8>& data, TPL textures, LevelData* level);
            static aiScene* LoadFromBytes(const u8* data, u64 size, TPL textures, LevelData* level);

        private:
            struct Vertex
            {
                Vector3 position;
                Color color;
                Vector3 normal;
                Vector2 uv;
            };

            static Section FindSection(const std::string& name, int sectionTableOffset, int sectionCount);
            static void ReadSection(Section section);
            static void ReadMaterialNameTable(int tableOffset, int textureCount);
            static aiNode* ReadInfo(int offset, std::vector<aiMesh*>& meshes);
            static aiNode* ReadObject(int objectOffset, int& nextSibling, std::vector<aiMesh*>& meshes, std::string indent = "");
            static aiMesh* ReadMesh(int offset);
            static void ReadFogTable(int offset);
            static void ReadAnimationTable(int tableOffset);
            static void ReadTextureAnimation(int offset);
            static void ReadTransformAnimation(int offset, aiAnimation* animation);
            static void ReadCurveTable(int offset);

            static std::vector<std::string> ReadTextureNames(int offset);
            static VCDTable ReadVCDTable(int offset);
            static void ReadVertices(VCDTable vcd, int offset, VertexAttributes attributes, std::vector<Vertex>& vertices, std::vector<int>& indices);


            static const u8* sData;
            static aiScene* sCurrentScene;
            static int s_FirstMaterialAddress; // Required to calculate the material index for each mesh
            static VCDTable s_VCDTable;
            static LevelData* s_CurrentLevel;
    };
}
