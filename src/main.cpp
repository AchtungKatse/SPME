#include <filesystem>
#include <iostream>
#include "Compressors/LZSS.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"
#include "FileTypes/U8Archive.h"
#include "FileTypes/LevelData.h"
#include "assimp/Exporter.hpp"
#include "assimp/postprocess.h"

using namespace std;
using namespace SPMEditor;

void U8(int& i, int argc, char** argv);
void U8Dump(string input, string output);
void U8Compile(string input, string output, bool compress);

void LZSSCommands(int& i, int argc, char** argv);
void ConvertCommand(int& i, int argc, char** argv);

constexpr unsigned int str2int(const char* str, int h = 0) {
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        string command = argv[i];
        switch (str2int(command.c_str()))
        {
            default:
                cout << "unrecognized command '" << command << "'" << endl;
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
                    cout << "u8 unrecognized argument '" << arg << "'" << endl;
                    i--;
                    return;
                }
            case str2int("--dump"):
                {
                    if (argc - i <= 2)
                    {
                        cout << "Incorrect format: u8 --dump <input file> <output directory>" << endl;
                        break;
                    }
                    string input = argv[i + 1];
                    string output = argv[i + 2];
                    U8Dump(input, output);

                    i += 2;
                    break;
                }
            case str2int("--compile"):
                {
                    if (argc - i <= 3)
                    {
                        cout << "Incorrect format: u8 --compile <input file> <output directory> <compressed (0 = false, 1 = true)>" << endl;
                        break;
                    }

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
    if (!filesystem::exists(input))
    {
        cout << "File '" << input << "' does not exist." << endl;
        return;
    }

    U8Archive archive = U8Archive::ReadFromFile(input);
    archive.Dump(output);
}

void U8Compile(string inputDirectory, string outputFile, bool compress) {
    if (!filesystem::exists(inputDirectory))
    {
        cout << "Directory '" << inputDirectory << "' does not exist." << endl;
        return;
    }

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
        cout << "Failed to create U8 archive from directory '" << inputDirectory << "'" << endl;
    }
}

void LZSSCommands(int& i, int argc, char** argv) {
    for (; i < argc; i++) {
        char* arg = argv[i];
        switch (str2int(arg))
        {
            default:
                {
                    i--;
                    return;
                }
            case str2int("--compress"):
                {
                    if (argc - i <= 2)
                    {
                        cout << "Incorrect format: lzss --compress <input file> <output file>" << endl;
                        break;
                    }

                    string input = argv[i + 1];
                    string output = argv[i + 2];

                    if (!filesystem::exists(input))
                    {
                        cout << "File '" << input << "' Does not exist." << endl;
                        break;
                    }

                    vector<u8> compressed = LZSS::CompressLzss11(FileReader::ReadFileBytes(input));
                    FileWriter::WriteFile(output, compressed);
                    i += 2;
                }
            case str2int("--decompress"):
                {
                    if (argc - i <= 2)
                    {
                        cout << "Incorrect format: lzss --decompress <input file> <output file>" << endl;
                        break;
                    }

                    string input = argv[i + 1];
                    string output = argv[i + 2];

                    if (!filesystem::exists(input))
                    {
                        cout << "File '" << input << "' Does not exist." << endl;
                        break;
                    }

                    vector<u8> decompressed = LZSS::DecompressBytes(FileReader::ReadFileBytes(input));
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
    level.files.Dump(level.name);

    U8Archive archive;
    if (U8Archive::TryCreateFromDirectory(level.name, archive))
    {
        // We need to recompress the map.dat file
        // FileWriter::WriteFile("he1_01-decompressed.bin", LZSS::DecompressBytes(FileReader::ReadFileBytes(inputFile)));
        string mapPath = level.name + "/dvd/map/" + level.name + "/map.dat";

        // Then write the archive to disk
        vector<u8> archiveData = archive.CompileU8();
        vector<u8> compressedArchive = LZSS::CompressLzss11(archiveData);
        FileWriter::WriteFile("new_he1_01.dat", compressedArchive);
        // FileWriter::WriteFile("new_he1_01.dat", archiveData);
    }

    cout << "------- Exporting Model -------" << endl;
    Assimp::Exporter exporter;
    const auto exportSuccess = exporter.Export(level.geometry, "fbx", level.name + ".fbx", aiProcess_ValidateDataStructure | aiProcess_EmbedTextures | aiProcess_GenSmoothNormals);

}
