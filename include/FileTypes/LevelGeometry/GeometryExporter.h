#pragma once
#include "assimp/scene.h"

namespace SPMEditor::GeometryExporter {
    void Write(aiScene* scene, const std::string& path);
}
