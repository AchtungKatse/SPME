#pragma once
#include "FileTypes/U8Archive.h"
#include "assimp/scene.h"

namespace SPMEditor {
    class LevelData
    {
        public:
            static LevelData LoadLevelFromFile(const std::string& path, bool compressed);
            static LevelData LoadLevelFromBytes(const std::string& name, const std::vector<u8>& data, bool compressed);

            U8Archive u8Files;
            std::string name;
            aiScene* geometry;
    };
}
