#include "Commands/Display/Display.h"
#include "Commands/LZSSCommands.h"
#include "Commands/U8Commands.h"
#include "Compressors/LZSS.h"
#include "IO/FileWriter.h"
#include "FileTypes/U8Archive.h"
#include "FileTypes/LevelData.h"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"
#include "Commands/TPLCommands.h"

using namespace SPMEditor;

void ConvertCommand(int& i, int argc, char** argv);

int main(int argc, char** argv) {
    spdlog::set_level(spdlog::level::trace);
    for (int i = 1; i < argc; i++) {
        std::string command = argv[i];
        switch (str2int(command.c_str()))
        {
            default:
                LogInfo("Unrecognized command '{}'", command);
                break;
            case str2int("tpl"):
                i++;
                TPLCommands::Read(i, argc, argv);
                break;
            case str2int("u8"):
                i++;
                U8Commands::Read(i, argc, argv);
                break;
            case str2int("lzss"):
                i++;
                LZSSCommands::Read(i, argc, argv);
                break;
            case str2int("convert"):
                i++;
                ConvertCommand(i, argc, argv);
                break;
            case str2int("display"):
                LevelData level = LevelData::LoadLevelFromFile(argv[i + 1], true);
                Display::DisplayLevel(level);
                break;
        }
    }

    return 0;
}

void ConvertCommand(int& i, int argc, char** argv) {
    Assert(i < argc, "Invalid number of arguments. (arg) {} >= (argc) ({}) {} \n\tUsage: convert <filepath>", i, argc, i < argc);
    const char* inputFile = argv[i];
    LogInfo("Converting file \"{}\"", inputFile);

    LevelData level = LevelData::LoadLevelFromFile(inputFile, true);
    level.u8Files.Dump(level.name);

    U8Archive archive;
    if (U8Archive::TryCreateFromDirectory(level.name, archive))
    {
        // We need to recompress the map.dat file
        // FileWriter::WriteFile("he1_01-decompressed.bin", LZSS::DecompressBytes(FileReader::ReadFileBytes(inputFile)));
        std::string mapPath = fmt::format("{0}/dvd/map/{0}/map.dat", level.name);

        // Then write the archive to disk
        std::vector<u8> archiveData = archive.CompileU8();
        std::vector<u8> compressedArchive = LZSS::CompressLzss10(archiveData);
        FileWriter::WriteFile(fmt::format("new_{}.dat", level.name), compressedArchive);
    }

    LogInfo("------- Exporting Model -------");
    Assimp::Exporter exporter;
    const auto exportSuccess = exporter.Export(level.geometry, "fbx", level.name + ".fbx", aiProcess_ValidateDataStructure | aiProcess_EmbedTextures | aiProcess_GenSmoothNormals);
}
