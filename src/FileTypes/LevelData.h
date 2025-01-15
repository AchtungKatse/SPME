#include "FileTypes/U8Archive.h"
#include "PCH.h"
#include "FileTypes/TPL.h"
#include "FileTypes/LevelGeometry/LevelGeometry.h"

namespace SPMEditor {
    struct LevelData
    {
        public:
            U8Archive files;
            string name;
            aiScene* geometry;

            static LevelData LoadLevelFromFile(string path, bool compressed);
            static LevelData LoadLevelFromBytes(string name, const vector<u8>& data, bool compressed);
    };
}
