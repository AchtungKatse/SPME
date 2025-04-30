#include "Commands/TPLCommands.h"
#include "FileTypes/TPL.h"
#include "Utility/str2int.h"
#include "stb_image_write.h"
#include <cstdio>
#include <filesystem>

namespace SPMEditor {
    
    void TPLCommands::Read(int& i, int argc, char** argv) {

        const char* command = argv[i++];
        switch (str2int(command)) {
            case str2int("dump"):
                Assert(i <= argc - 2, "Invalid parameter count");
                Dump(argv[i], argv[i + 1]);
                i++;
                break;
        }
    }

    void TPLCommands::Dump(const char* inputFile, const char* outputDirectory) {
        if (!std::filesystem::exists(outputDirectory))
            std::filesystem::create_directory(outputDirectory);

        LogInfo("Dumping tpl '%s' to '%s'", inputFile, outputDirectory);
        TPL tpl = TPL::LoadFromFile(inputFile);

        for (size_t i = 0; i < tpl.images.size(); i++) {
            auto& image = tpl.images[i];
            char finalName[0x200] = {};
            if (image.name == "") {
                snprintf(finalName, sizeof(finalName), "Image_%lu", i);
            } else {
                strcpy(finalName, image.name.c_str());
            }
            LogInfo("Writing image '%s/%s.png'", outputDirectory, finalName);

            char path[0x300] = {};
            snprintf(path, sizeof(path), "%s/%s.png", outputDirectory, finalName);
            stbi_write_png(path, (int)image.header.width, (int)image.header.height, 4, image.pixels.data(), image.header.width * 4);
        }
    }
}
