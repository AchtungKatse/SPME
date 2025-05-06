#include "FileTypes/MapConfig.h"
#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <ostream>

namespace YAML {
    template<>
        struct convert<SPMEditor::MaterialConfig> {
            static Node encode(const SPMEditor::MaterialConfig& rhs) {
                Node node;
                node["Name"] = rhs.mName;
                node["UseTransparency"] = rhs.mUseTransparency;
                node["UseVertexColor"] = rhs.mUseVertexColor;
                return node;
            }

            static bool decode(const Node& node, SPMEditor::MaterialConfig& rhs) {
                rhs.mName = node["Name"].as<std::string>();
                rhs.mUseTransparency = node["UseTransparency"].as<bool>();
                rhs.mUseVertexColor = node["UseVertexColor"].as<bool>();
                return true;
            }
        };

    template<>
        struct convert<SPMEditor::TextureConfig> {
            static Node encode(const SPMEditor::TextureConfig& rhs) {
                Node node;
                node["Name"] = rhs.mName;
                node["UseTransparency"] = rhs.mUseTransparency;
                node["WrapModeU"] = (u32)rhs.mWrapModeU;
                node["WrapModeV"] = (u32)rhs.mWrapModeV;
                return node;
            }

            static bool decode(const Node& node, SPMEditor::TextureConfig& rhs) {
                rhs.mName = node["Name"].as<std::string>();
                rhs.mUseTransparency = node["UseTransparency"].as<bool>();
                rhs.mWrapModeU = (SPMEditor::MapTexture::WrapMode)node["WrapModeU"].as<u32>();
                rhs.mWrapModeV = (SPMEditor::MapTexture::WrapMode)node["WrapModeV"].as<u32>();
                return true;
            }
        };
}

namespace SPMEditor {
    // Private functions
    void WriteMapConfigToFile(const MapConfig* config, const char* outFile);

    void MapConfig::CreateFromModel(const char* modelPath, const char* outputPath) {
        // Load the scene
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(modelPath, aiPostProcessSteps::aiProcess_Triangulate | aiProcess_FlipWindingOrder | aiPostProcessSteps::aiProcess_EmbedTextures | aiPostProcessSteps::aiProcess_ValidateDataStructure | aiPostProcessSteps::aiProcess_FindInvalidData | aiPostProcessSteps::aiProcess_ForceGenNormals | aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords);

        // Create default texture configs
        std::vector<TextureConfig> textureConfigs;
        textureConfigs.reserve(scene->mNumTextures);
        for (u32 i = 0; i < scene->mNumTextures; i++) {
            aiTexture* tex = scene->mTextures[i];
            textureConfigs.emplace_back((TextureConfig) {
                    .mName = tex->mFilename.C_Str(),
                    .mUseTransparency = false,
                    .mWrapModeU = MapTexture::WrapMode::Repeat,
                    .mWrapModeV = MapTexture::WrapMode::Repeat,
                    });
        }

        // Create default material configs
        std::vector<MaterialConfig> materialConfigs;
        materialConfigs.reserve(scene->mNumMaterials);
        for (u32 i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];
            LogDebug("Writing material config: '%s'", mat->GetName().C_Str());
            materialConfigs.emplace_back((MaterialConfig) {
                    .mName = mat->GetName().C_Str(),
                    .mUseTransparency = false,
                    .mUseVertexColor = true,
                    });
        }

        // Create Map Config
        MapConfig config = {
            .mMapName = scene->mName.C_Str(),
            .mTextureConfigs = textureConfigs,
            .mMaterialConfigs = materialConfigs,
        };

        WriteMapConfigToFile(&config, outputPath);
    }

    MapConfig MapConfig::LoadFromFile(const char* filePath) {
        YAML::Node yaml = YAML::LoadFile(filePath);
        MapConfig config; 

        config.mTextureConfigs = yaml["TextureConfigs"].as<std::vector<TextureConfig>>();
        config.mMaterialConfigs = yaml["MaterialConfigs"].as<std::vector<MaterialConfig>>();
        config.mMapName = yaml["MapName"].as<std::string>();

        return config;
    }

    void WriteMapConfigToFile(const char* mapName, const MapConfig* config, const char* outFile) {
        YAML::Node node;
        node["TextureConfigs"] = config->mTextureConfigs;
        node["MaterialConfigs"] = config->mMaterialConfigs;
        node["MapName"] = mapName;

        std::ofstream outputStream(outFile);
        outputStream << node;
        LogInfo("Write map config to file '%s'", outFile);
    }
}
