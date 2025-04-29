#include "Commands/CommandType.h"
#include "Commands/Display/Display.h"
#include "Commands/LZSSCommands.h"
#include "Commands/U8Commands.h"
#include "FileTypes/LevelGeometry/GeometryExporter.h"
#include "FileTypes/LevelData.h"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"
#include "Commands/TPLCommands.h"
#include "assimp/Importer.hpp"

using namespace SPMEditor;
std::array<SPMEditor::CommandGroup*, 1> commands = {
    new LZSSCommands(),
};

void ConvertCommand(int argc, char** argv);
char ToLower(char c) {
    if (c >= 'A' && c <= 'Z') 
        return c + ('a' - 'A');
    return c;
}

std::string ToLower(const std::string& string) {
    std::string out;
    out.reserve(string.size());

    for (const char c : string) {
        out += ToLower(c);
    }

    return out;
}

int main(int argc, char** argv) {
    spdlog::set_level(spdlog::level::trace);
    for (int i = 1; i < argc; i++) {
        std::string arg = ToLower(argv[i]);

        bool foundCommand = false;
        for (CommandGroup* group : commands) {
            const std::string groupName = ToLower(group->GetName());
            if (groupName != arg)
                continue;

            const std::string commandName = ToLower(argv[i + 1]);
            for (const Command* command : group->GetCommands()) {
                const std::string currentCommandName = ToLower(command->GetName());

                if (currentCommandName != commandName) 
                    continue;

                if (i + command->GetParameterCount() >= argc) {
                    LogError("Failed to read command '{}'. Invalid number of arguments. Expected {}, got {}", commandName, command->GetParameterCount(), argc - i - 1);
                    abort();
                }

                foundCommand = true;
                command->Run(argv + i + 2); // add 2 since [command group] [command]
                i += command->GetParameterCount();
            }
        }

        if (foundCommand)
            continue;

        switch (str2int(arg.c_str()))
        {
            default:
                LogInfo("Unrecognized command '{}'", arg);
                break;
            case str2int("tpl"):
                i++;
                TPLCommands::Read(i, argc, argv);
                break;
            case str2int("u8"):
                i++;
                U8Commands::Read(i, argc, argv);
                break;
            case str2int("convert"):
                i++;
                ConvertCommand(argc - i, argv + i);
                break;
            case str2int("export"):
                {
                    const std::string input = argv[i + 1];
                    const std::string output = argv[i + 2];
                    Assert(std::filesystem::exists(input), "Map directory '{}' does not exist", input);
                    Assert(std::filesystem::is_regular_file(input), "Map directory '{}' is not a regular file.", input);

                    GeometryExporter* exporter = GeometryExporter::Create();
                    Assimp::Importer importer;
                    const aiScene* scene = importer.ReadFile(input.c_str(), aiPostProcessSteps::aiProcess_Triangulate | aiProcess_FlipWindingOrder | aiPostProcessSteps::aiProcess_EmbedTextures | aiPostProcessSteps::aiProcess_ValidateDataStructure | aiPostProcessSteps::aiProcess_FindInvalidData | aiPostProcessSteps::aiProcess_ForceGenNormals | aiProcess_JoinIdenticalVertices | aiProcess_GenUVCoords);
                    Assert(scene, "Failed to load scene '{}'", input.c_str());
                    exporter->Write(scene, output);
                    break;
                }
#ifndef NO_BUILD_DISPLAY
            case str2int("display"):
                const std::string fileName = argv[i + 1];
                if (!std::filesystem::exists(fileName))
                    continue;
                if (!std::filesystem::is_regular_file(fileName))
                    continue;

                LevelData level = LevelData::LoadLevelFromFile(argv[i + 1], true);
                Display::DisplayLevel(level);
                break;
#endif
        }
    }

    return 0;
}

void ConvertCommand(int argc, char** argv) {
    Assert(argc >= 2, "Convert command requrires two arguments, {} provided. \n\tUsage: convert <filepath> <Output format (i.e. fbx, glb)> <Map Name [Optional]>", argc);
    const char* inputFile = argv[0];
    const char* format = argv[1];
    LogInfo("Converting file \"{}\"", inputFile);

    const char* mapName = "";
    if (argc > 2) {
        mapName = argv[2];
    }

    LevelData level = LevelData::LoadLevelFromFile(inputFile, true, mapName);

    LogInfo("------- Exporting Model -------");
    Assimp::Exporter exporter;
    const auto exportSuccess = exporter.Export(level.geometry, format, level.name + "." + format, aiProcess_EmbedTextures | aiProcess_Triangulate | aiProcess_GenBoundingBoxes | aiProcess_FlipWindingOrder);
    Assert(exportSuccess == aiReturn_SUCCESS, "Failed to export {}", level.name);
}
