#include "FileTypes/LevelData.h"
#include "FileTypes/LevelGeometry/LevelGeometry.h"
#include "FileTypes/U8Archive.h"
#include "IO/FileReader.h"
#include "assimp/material.h"
#include <cstdio>

namespace SPMEditor {
    LevelData LevelData::LoadLevelFromFile(const std::string& path, bool compressed, const std::string& mapNameOverride) {
        Assert(std::filesystem::exists(path), "Failed to find file %s", path.c_str());
        std::filesystem::path filePath(path);
        FileHandle handle = FileReader::ReadFileBytes(path);
        
        const std::string& fileName = filePath.filename().string();
        const std::string& name = fileName.substr(0, fileName.size() - 4);

        LogInfo("Loaded file '%s'", fileName.c_str());

        return LoadLevelFromBytes(name, handle.data, handle.size, compressed, mapNameOverride);
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
        U8File& textureFile = level.u8Files[texturePath];
        TPL tpl = TPL::LoadFromBytes(textureFile.data, textureFile.size);

        // Load main map data
        char mapPath[0x200] = {};
        snprintf(mapPath, sizeof(mapPath), "./dvd/map/%s/map.dat", mapName.c_str());
        if (!level.u8Files.Exists(mapPath))
        {
            LogError("Level does not contain path '%s'", mapPath);
            return level;
        }
        const U8File& map = level.u8Files[mapPath];
        level.geometry = LevelGeometry::LoadFromBytes(map.data, map.size, tpl, &level);

        return level;
    }
}
