#include "FileTypes/LevelData.h"
#include "FileTypes/LevelGeometry/LevelGeometry.h"
#include "FileTypes/U8Archive.h"
#include "assimp/material.h"
#include "core/filesystem.h"
#include <cstdio>
#include <filesystem>

namespace SPMEditor {
    LevelData LevelData::LoadLevelFromFile(const std::string& path, bool compressed, const std::string& mapNameOverride) {
        Assert(std::filesystem::exists(path), "Failed to find file %s", path.c_str());
        std::filesystem::path filePath(path);
        FileHandle handle = filesystem_read_file(path.c_str());
        
        const std::string& fileName = filePath.filename().string();
        const std::string& name = fileName.substr(0, fileName.size() - 4);

        LogInfo("Loaded file '%s'", fileName.c_str());

        return LevelData::LoadLevelFromBytes(name, (const u8*)handle.data, handle.size, compressed, mapNameOverride);
    }

    void ReadMat(aiMaterial* mat) {
        LogInfo("Reading material: %s", mat->GetName().C_Str());
        for (size_t i = 0; i < mat->mNumProperties; i++) {
            aiMaterialProperty* prop = mat->mProperties[i];
            LogInfo("\tProperty Key: %s", prop->mKey.C_Str());

            if (prop->mType == aiPropertyTypeInfo::aiPTI_String)
                LogInfo("\tText: '%s'", ((aiString*)prop->mData)->C_Str());
            if (prop->mType == aiPropertyTypeInfo::aiPTI_Integer)
                LogInfo("\tValue: %d", *((int*)prop->mData));

            LogInfo("\n\t\tType: %d; Index: %d; Length: %d; Semantic: %du", (int)prop->mType, prop->mIndex, prop->mDataLength, prop->mSemantic);
        }
    }

    LevelData LevelData::LoadLevelFromBytes(const std::string& name, const std::vector<u8>& data, bool compressed, const std::string& mapNameOverride) {
        return LoadLevelFromBytes(name, data.data(), data.size(), compressed, mapNameOverride);
    }

    LevelData LevelData::LoadLevelFromBytes(const std::string& name, const u8* data, u64 size, bool compressed, const std::string& mapNameOverride) {
        const auto baseArchive = U8Archive::ReadFromBytes(data, size, compressed);

        std::string mapName;
        if (mapNameOverride != "") {
            mapName = mapNameOverride;
        } else {
            mapName = name; 
        }

        LevelData level;
        level.u8Files = baseArchive;
        level.name = mapName;

        // Load level geometry
        // Grab texturesE
        std::string texturePath = "./dvd/map/" + mapName + "/texture.tpl";
        Assert(level.u8Files.Exists(texturePath), "Level does not have file at path '%s'", texturePath.c_str());
        U8File* textureFile = nullptr;
        if (!level.u8Files.Get(texturePath, &textureFile)) {
            LogError("Cannot load level data. Failed to get texture at path %s", texturePath.c_str());
            return LevelData();
        }
        TPL tpl = TPL::LoadFromBytes(textureFile->data, textureFile->size);

        // Load main map data
        char mapPath[0x200] = {};
        snprintf(mapPath, sizeof(mapPath), "./dvd/map/%s/map.dat", mapName.c_str());
        if (!level.u8Files.Exists(mapPath))
        {
            LogError("Level does not contain path '%s'", mapPath);
            return level;
        }
        U8File* mapFile = nullptr;
        if (!level.u8Files.Get(mapPath, &mapFile)) {
            LogError("Cannot load level data. Failed to get map.dat at path %s", mapPath);
            return LevelData();
        }
        level.geometry = LevelGeometry::LoadFromBytes(mapFile->data, mapFile->size, tpl, &level);

        return level;
    }
}
