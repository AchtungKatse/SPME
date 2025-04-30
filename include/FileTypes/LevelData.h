#pragma once
#include "FileTypes/LevelGeometry/InternalMapFile.h"
#include "FileTypes/U8Archive.h"
#include "assimp/scene.h"
#include <vector>

namespace SPMEditor {
    class LevelData {
        public:
            static LevelData LoadLevelFromFile(const std::string& path, bool compressed, const std::string& mapNameOverride = "");
            static LevelData LoadLevelFromBytes(const std::string& name, const std::vector<u8>& data, bool compressed, const std::string& mapNameOverride = "");
            static LevelData LoadLevelFromBytes(const std::string& name, const u8* data, u64 size, bool compressed, const std::string& mapNameOverride = "");

            U8Archive u8Files;
            std::string name;
            aiScene* geometry;
            std::vector<LevelInternal::FogEntry> fogSettings;
    };
}
