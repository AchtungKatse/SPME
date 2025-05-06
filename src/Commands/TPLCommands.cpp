#include "Commands/TPLCommands.h"
#include "FileTypes/TPL.h"
#include "stb_image_write.h"
#include <cstdio>
#include <filesystem>

namespace SPMEditor::TPLCommands {
    void Dump(u32 argc, const char** argv) {
        const char* input_file = argv[0];
        const char* output_directory = argv[1];

        if (!std::filesystem::exists(output_directory))
            std::filesystem::create_directory(output_directory);

        LogInfo("Dumping tpl '%s' to '%s'", input_file, output_directory);
        SPMEditor::TPL tpl = SPMEditor::TPL::LoadFromFile(input_file);

        for (size_t i = 0; i < tpl.images.size(); i++) {
            auto& image = tpl.images[i];
            char finalName[0x200] = {};
            if (image.name == "") {
                snprintf(finalName, sizeof(finalName), "Image_%lu", i);
            } else {
                strcpy(finalName, image.name.c_str());
            }
            LogInfo("Writing image '%s/%s.png'", output_directory, finalName);

            char path[0x300] = {};
            snprintf(path, sizeof(path), "%s/%s.png", output_directory, finalName);
            stbi_write_png(path, (int)image.header.width, (int)image.header.height, 4, image.pixels.data(), image.header.width * 4);
        }
    }
}
