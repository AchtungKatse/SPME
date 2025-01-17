#include "Commands/TPLCommands.h"
#include "FileTypes/TPL.h"
#include "Utility/str2int.h"
#include "stb_image_write.h"
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

        LogInfo("Dumping tpl '{}' to '{}'", inputFile, outputDirectory);
        TPL tpl = TPL::LoadFromFile(inputFile);

        for (int i = 0; i < tpl.images.size(); i++) {
            auto& image = tpl.images[i];
            std::string& name = tpl.images[i].name;
            if (image.name == "")
            {
                name = fmt::format("Image_{}", i);
            }
            LogInfo("Writing image '{}/{}.png'", outputDirectory, name);
            stbi_write_png(fmt::format("{}/{}.png", outputDirectory, name).c_str(), (int)image.header.width, (int)image.header.height, 4, image.pixels.data(), image.header.width * 4);
        }
    }
}
