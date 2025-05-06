#pragma once

#include "FileTypes/LevelGeometry/MapStructures.h"

#include <string>
#include <vector>

namespace SPMEditor {
    using namespace MapStructures;

    // NOTE: For whatever reason, blender's gltf export results in all images having the name "Image-Image". 
    // This doesn't seem to have an effect on SPM, but it does make it very annoying since it prevents modifying an existing config based on texture names
    struct TextureConfig {
        std::string mName;
        bool mUseTransparency;
        MapTexture::WrapMode mWrapModeU;
        MapTexture::WrapMode mWrapModeV;
    };

    struct MaterialConfig {
        std::string mName;
        bool mUseTransparency;
        bool mUseVertexColor;
    };

    struct MapConfig {
        std::string mMapName;
        std::vector<TextureConfig> mTextureConfigs;
        std::vector<MaterialConfig> mMaterialConfigs;

        /**
         * @brief Creates a config from an existing model (i.e. he1_01.glb)
         *
         * @param modelPath The path to the model
         * @param outputPath The path to write the output yaml config
         */
        static void CreateFromModel(const char* mapName, const char* modelPath, const char* outputPath);
        // NOTE: UpdateFromModel removed for the above reason (Image names causing issues)
        /**
         * @brief Updates a config from an existing model (i.e. he1_01.glb)
         *
         * @param modelPath The path to the model
         * @param outputPath The path to write the output yaml config
         */
        /*static void UpdateFromModel(const char* modelPath, const char* outputPath);*/
        /**
         * @brief Loads MapConfig from a yaml config file
         *
         * @param filePath The path to the config file
         * @return 
         */
        static MapConfig LoadFromFile(const char* filePath);
    };
}
