#pragma once
#include "FileTypes/TPL.h"
#include "assimp/scene.h"

namespace SPMEditor {
    struct LevelGeometry
    {
        public:
            static aiScene* LoadFromBytes(vector<u8> data, TPL textures);
    };
}
