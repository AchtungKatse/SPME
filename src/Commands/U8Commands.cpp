#include "Commands/U8Commands.h"
#include "FileTypes/U8Archive.h"
#include "IO/FileWriter.h"
#include "Compressors/LZSS.h"

namespace SPMEditor {
    void U8Commands::Read(int& i, int argc, char** argv) {
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
                case str2int("dump"):
                    {
                        Assert(i < argc - 2, "Incorrect format: u8 --dump <input file> <output directory>");
                        std::string input = argv[i + 1];
                        std::string output = argv[i + 2];

                        Assert(std::filesystem::exists(input), "File '{}' does not exist.", input);
                        U8Archive archive = U8Archive::ReadFromFile(input, true);
                        archive.Dump(output);

                        i += 2;
                        break;
                    }
                case str2int("compile"):
                    {
                        Assert(i + 3 < argc, "Incorrect format: u8 --compile <input file> <output directory> <compressed (0 = false, 1 = true)>");

                        std::string input = argv[i + 1];
                        std::string output = argv[i + 2];
                        std::string compressed = argv[i + 3];

                        Assert(std::filesystem::exists(input), "Directory '{}' does not exist.", input);

                        U8Archive archive;
                        Assert(U8Archive::TryCreateFromDirectory(input, archive), "Failed to create U8 archive from directory '{}'", input);
                        std::vector<u8> data = archive.CompileU8();

                        if (compressed == "1" || compressed == "true")
                            data = LZSS::CompressLzss10(data);

                        FileWriter::WriteFile(output, data);

                        i += 3;
                        break;
                    }
            }
        }
    }
}
