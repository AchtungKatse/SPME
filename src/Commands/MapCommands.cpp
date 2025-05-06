#include "Commands/MapCommands.h"
#include <filesystem>
#include "FileTypes/LevelData.h"
#include "FileTypes/LevelGeometry/GeometryExporter.h"
#include <assimp/Exporter.hpp>
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

namespace SPMEditor::MapCommands {

    void ToFBX(u32 argc, const char** argv) {
        const char* inputFile = argv[0];
        const char* outputFile = argv[1];
        LogInfo("Converting file \"%s\"", inputFile);

        const char* mapName = "";
        if (argc > 2) {
            mapName = argv[2];
        }

        LevelData level = LevelData::LoadLevelFromFile(inputFile, true, mapName);

        LogInfo("------- Exporting Model -------");
        Assimp::Exporter exporter;
        const aiReturn exportSuccess = exporter.Export(level.geometry, "fbx", outputFile, aiProcess_EmbedTextures | aiProcess_Triangulate | aiProcess_GenBoundingBoxes | aiProcess_FlipWindingOrder);
        Assert(exportSuccess == aiReturn_SUCCESS, "Failed to export %s", level.name.c_str());
    }

    void FromGLB(u32 argc, const char** argv) {
        const char* input = argv[0];
        const char* configPath = argv[1];
        const char* output = argv[2];
        Assert(std::filesystem::exists(input), "Export Failed. Map directory '%s' does not exist", input);
        Assert(std::filesystem::exists(configPath), "Export Failed. Cannot find config at path '%s'", configPath);
        Assert(std::filesystem::is_regular_file(input), "Export Failed. Map directory '%s' is not a regular file.", input);
        Assert(std::filesystem::is_regular_file(configPath), "Export Failed. Config '%s' is not a regular file.", input);

        MapConfig config = MapConfig::LoadFromFile(configPath);

        GeometryExporter* exporter = GeometryExporter::Create();
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(input, aiPostProcessSteps::aiProcess_Triangulate | aiProcess_FlipWindingOrder | aiPostProcessSteps::aiProcess_EmbedTextures | aiPostProcessSteps::aiProcess_ValidateDataStructure | aiPostProcessSteps::aiProcess_FindInvalidData | aiPostProcessSteps::aiProcess_ForceGenNormals | aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords);
        Assert(scene, "Export failed. Assimp failed to load scene '%s'. Supported formats are GLB, GLTF, and FBX.", input);
        exporter->Write(scene, config, output);
    }

    void CreateConfig(u32 argc, const char** argv) {
        const char* mapName = argv[0];
        const char* modelPath = argv[1];
        const char* configPath = argv[2];
        Assert(std::filesystem::exists(modelPath), "Failed to create map config. Map directory '%s' does not exist", modelPath);
        Assert(std::filesystem::is_regular_file(modelPath), "Failed to create map config. Map directory '%s' is not a regular file.", modelPath);

        MapConfig::CreateFromModel(mapName, modelPath, configPath);
    }
}

