#include "Commands/LZSSCommands.h"
#include "IO/FileReader.h"
#include "Compressors/LZSS.h"
#include "IO/FileWriter.h"

namespace SPMEditor {
    void LZSSCommands::Read(int& i, int argc, char** argv) {
        for (; i < argc; i++) {
            char* arg = argv[i];
            switch (str2int(arg))
            {
                default:
                    i--;
                    return;
                case str2int("compress"):
                    {
                        Assert(i + 2 < argc, "Incorrect format: lzss --compress <input file> <output file>");

                        const std::string& input = argv[i + 1];
                        const std::string& output = argv[i + 2];

                        Assert(std::filesystem::exists(input), "File '{}' Does not exist.", input);

                        const std::vector<u8> compressed = LZSS::CompressLzss10(FileReader::ReadFileBytes(input));
                        FileWriter::WriteFile(output, compressed);
                        i += 2;
                        break;
                    }
                case str2int("decompress"):
                    {
                        Assert(i + 2 < argc, "Incorrect format: lzss --decompress <input file> <output file>");

                        const std::string& input = argv[i + 1];
                        const std::string& output = argv[i + 2];

                        Assert(std::filesystem::exists(input), "File '{}' Does not exist.", input);

                        const std::vector<u8>& compressed = FileReader::ReadFileBytes(input);
                        const std::vector<u8> decompressed = LZSS::DecompressBytes(compressed.data(), compressed.size());
                        FileWriter::WriteFile(output, decompressed);
                        i += 2;
                        break;
                    }

            }
        }
    }
}
