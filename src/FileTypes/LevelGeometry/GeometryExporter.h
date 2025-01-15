#pragma once
#include "PCH.h"
#include "FileTypes/LevelGeometry/LevelGeometry.h"
#include "assimp/scene.h"

namespace SPMEditor::GeometryExporter {
    void Write(aiScene* scene, string path);
}
