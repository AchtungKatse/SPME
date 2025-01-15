#include "LevelData.h"
#include "FileTypes/LevelGeometry/LevelGeometry.h"
#include "FileTypes/U8Archive.h"
#include "IO/FileReader.h"

namespace SPMEditor {
    LevelData LevelData::LoadLevelFromFile(string path, bool compressed)
    {
        Assert(std::filesystem::exists(path), "Failed to find file {}", path);
        filesystem::path filePath(path);
        const vector<u8>& data = FileReader::ReadFileBytes(path);
        
        string fileName = filePath.filename();
        string name = fileName.substr(0, fileName.size() - 4);

        LogInfo("Loaded file '{}'", fileName);

        return LoadLevelFromBytes(name, data, compressed);
    }

    void ReadMat(aiMaterial* mat)
    {
        cout << "Reading material: " << mat->GetName().C_Str() << endl;
        for (int i = 0; i < mat->mNumProperties; i++) {
            auto prop = mat->mProperties[i];
            cout << "\tProperty Key: " << prop->mKey.C_Str();

            if (prop->mType == 3)
                cout << "; Text: '" << ((aiString*)prop->mData)->C_Str() << "'";
            if (prop->mType == 4)
                cout << "; Value: " << *((int*)prop->mData);

            cout << "\n\t\tType: " << prop->mType << "; Index: " << prop->mIndex << "; Length: " << prop->mDataLength << "; Semantic: " << prop->mSemantic; 
            cout << endl << endl;
        }
    }

    LevelData LevelData::LoadLevelFromBytes(string name, const vector<u8>& data, bool compressed)
    {
        const auto baseArchive = U8Archive::ReadFromBytes(data);

        LevelData level;
        level.files = baseArchive;
        level.name = name;

        // Load level geometry
        // Grab texturesE
        U8Archive::File& textureFile = level.files["./dvd/map/" + name + "/texture.tpl"];
        TPL tpl = TPL::LoadFromBytes(textureFile.data);

        // Load main map data
        string mapPath = "./dvd/map/" + name + "/map.dat";
        if (!level.files.Exists(mapPath))
        {
            for (auto pair : level.files.files)
            {
                cout << "\t" << pair.first << endl;
            }
            cout << "Level does not contain path '" << mapPath << "'" << endl;
            return level;
        }
        auto map = level.files[mapPath];
        level.geometry = LevelGeometry::LoadFromBytes(map.data, tpl);

        return level;
    }
}
