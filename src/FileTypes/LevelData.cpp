#include "FileTypes/LevelData.h"
#include "FileTypes/LevelGeometry/LevelGeometry.h"
#include "FileTypes/U8Archive.h"
#include "IO/FileReader.h"
#include "assimp/material.h"

namespace SPMEditor {
    LevelData LevelData::LoadLevelFromFile(const std::string& path, bool compressed)
    {
        Assert(std::filesystem::exists(path), "Failed to find file {}", path);
        std::filesystem::path filePath(path);
        const std::vector<u8>& data = FileReader::ReadFileBytes(path);
        
        const std::string& fileName = filePath.filename();
        const std::string& name = fileName.substr(0, fileName.size() - 4);

        LogInfo("Loaded file '{}'", fileName);

        return LoadLevelFromBytes(name, data, compressed);
    }

    void ReadMat(aiMaterial* mat)
    {
        LogInfo("Reading material: {}", mat->GetName().C_Str());
        for (int i = 0; i < mat->mNumProperties; i++) {
            auto prop = mat->mProperties[i];
            LogInfo("\tProperty Key: ", prop->mKey.C_Str());

            if (prop->mType == aiPropertyTypeInfo::aiPTI_String)
                LogInfo("\tText: '{}'", ((aiString*)prop->mData)->C_Str());
            if (prop->mType == aiPropertyTypeInfo::aiPTI_Integer)
                LogInfo("\tValue: ", *((int*)prop->mData));

            LogInfo("\n\t\tType: {}; Index: {}; Length: {}; Semantic: {}", (int)prop->mType, prop->mIndex, prop->mDataLength, prop->mSemantic);
        }
    }

    LevelData LevelData::LoadLevelFromBytes(const std::string& name, const std::vector<u8>& data, bool compressed)
    {
        const auto baseArchive = U8Archive::ReadFromBytes(data, compressed);

        LevelData level;
        level.u8Files = baseArchive;
        level.name = name;

        // Load level geometry
        // Grab texturesE
        U8Archive::File& textureFile = level.u8Files["./dvd/map/" + name + "/texture.tpl"];
        TPL tpl = TPL::LoadFromBytes(textureFile.data);

        // Load main map data
        const std::string& mapPath = fmt::format("./dvd/map/{}/map.dat", name);
        if (!level.u8Files.Exists(mapPath))
        {
            for (auto pair : level.u8Files.files)
            {
                LogError("\t{}", pair.first);
            }

            LogError("Level does not contain path '{}'", mapPath);
            return level;
        }
        auto map = level.u8Files[mapPath];
        level.geometry = LevelGeometry::LoadFromBytes(map.data, tpl);

        return level;
    }
}
