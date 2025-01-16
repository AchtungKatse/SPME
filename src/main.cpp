#include <filesystem>
#include "Compressors/LZSS.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"
#include "FileTypes/U8Archive.h"
#include "FileTypes/LevelData.h"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"
#include "spdlog/common.h"

using namespace SPMEditor;
using namespace std;

void U8(int& i, int argc, char** argv);
void U8Dump(string input, string output);
void U8Compile(string input, string output, bool compress);

void LZSSCommands(int& i, int argc, char** argv);
void ConvertCommand(int& i, int argc, char** argv);

int main(int argc, char** argv) {
    spdlog::set_level(spdlog::level::trace);
    for (int i = 1; i < argc; i++) {
        string command = argv[i];
        switch (str2int(command.c_str()))
        {
            default:
                LogInfo("Unrecognized command '{}'", command);
                break;
            case str2int("u8"):
                i++;
                U8(i, argc, argv);
                break;
            case str2int("lzss"):
                i++;
                LZSSCommands(i, argc, argv);
                break;
            case str2int("convert"):
                i++;
                ConvertCommand(i, argc, argv);
                break;

        }
    }

    return 0;
}

void U8(int& i, int argc, char** argv) {
    for (; i < argc; i++) {
        char* arg = argv[i];
        switch (str2int(arg))
        {
            default:
                {
                    LogError("U8 unrecognized argument '{}'", arg); 
                    i--;
                    return;
                }
            case str2int("--dump"):
                {
                    Assert(argc - i <= 2, "Incorrect format: u8 --dump <input file> <output directory>");
                    string input = argv[i + 1];
                    string output = argv[i + 2];
                    U8Dump(input, output);

                    i += 2;
                    break;
                }
            case str2int("--compile"):
                {
                    Assert(argc - i <= 3, "Incorrect format: u8 --compile <input file> <output directory> <compressed (0 = false, 1 = true)>");

                    string input = argv[i + 1];
                    string output = argv[i + 2];
                    string compressed = argv[i + 3];
                    U8Compile(input, output, compressed == "1");

                    i += 3;
                    break;
                }
        }
    }
}

void U8Dump(string input, string output) {
    Assert(std::filesystem::exists(input), "File '{}' does not exist.", input);
    U8Archive archive = U8Archive::ReadFromFile(input, true);
    archive.Dump(output);
}

void U8Compile(string inputDirectory, string outputFile, bool compress) {
    Assert(std::filesystem::exists(inputDirectory), "Directory '{}' does not exist.", inputDirectory);

    U8Archive archive;
    if (U8Archive::TryCreateFromDirectory(inputDirectory, archive))
    {
        vector<u8> data = archive.CompileU8();

        if (compress)
            data = LZSS::CompressLzss11(data);

        FileWriter::WriteFile(outputFile, data);
    }
    else
    {
        LogError("Failed to create U8 archive from directory '{}'", inputDirectory);
    }
}

void LZSSCommands(int& i, int argc, char** argv) {
    for (; i < argc; i++) {
        char* arg = argv[i];
        switch (str2int(arg))
        {
            default:
                i--;
                return;
            case str2int("--compress"):
                {
                    Assert(argc - i <= 2, "Incorrect format: lzss --compress <input file> <output file>");

                    string input = argv[i + 1];
                    string output = argv[i + 2];

                    Assert(std::filesystem::exists(input), "File '{}' Does not exist.", input);

                    vector<u8> compressed = LZSS::CompressLzss11(FileReader::ReadFileBytes(input));
                    FileWriter::WriteFile(output, compressed);
                    i += 2;
                }
            case str2int("--decompress"):
                {
                    Assert(argc - i <= 2, "Incorrect format: lzss --decompress <input file> <output file>");

                    string input = argv[i + 1];
                    string output = argv[i + 2];

                    Assert(std::filesystem::exists(input), "File '{}' Does not exist.", input);

                    const std::vector<u8>& compressed = FileReader::ReadFileBytes(input);
                    vector<u8> decompressed = LZSS::DecompressBytes(compressed.data(), compressed.size());
                    FileWriter::WriteFile(output, decompressed);
                    i += 2;
                }

        }
    }
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
        string mapPath = fmt::format("{0}/dvd/map/{0}/map.dat", level.name);

        // Then write the archive to disk
        vector<u8> archiveData = archive.CompileU8();
        vector<u8> compressedArchive = LZSS::CompressLzss11(archiveData);
        FileWriter::WriteFile(fmt::format("new_{}.dat", level.name), compressedArchive);
    }

    LogInfo("------- Exporting Model -------");
    Assimp::Exporter exporter;
    const auto exportSuccess = exporter.Export(level.geometry, "fbx", level.name + ".fbx", aiProcess_ValidateDataStructure | aiProcess_EmbedTextures | aiProcess_GenSmoothNormals);

}
