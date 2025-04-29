#include "Commands/LZSSCommands.h"
#include "IO/FileHandle.h"
#include "IO/FileReader.h"
#include "Compressors/LZSS.h"
#include "IO/FileWriter.h"

namespace SPMEditor {
    const std::vector<Command*> LZSSCommands::sCommands = {
        new Decompress(),
        new Compress(),
    };

    const std::string& LZSSCommands::GetName() const {
        static const std::string mName = "LZSS";
        return mName;
    }

    const std::vector<Command*>& LZSSCommands::GetCommands() const {
        return sCommands;
    }

    void LZSSCommands::Decompress::Run(char** argv) const {
        const std::string& input = argv[0];
        const std::string& output = argv[1];

        Assert(std::filesystem::exists(input), "File '{}' Does not exist.", input);

        const FileHandle& compressed = FileReader::ReadFileBytes(input);
        const std::vector<u8> decompressed = LZSS::DecompressBytes(compressed.data, compressed.size);
        FileWriter::WriteFile(output, decompressed);
    }

    void LZSSCommands::Compress::Run(char** argv) const {
        const std::string& input = argv[0];
        const std::string& output = argv[1];

        Assert(std::filesystem::exists(input), "File '{}' Does not exist.", input);

        FileHandle file = FileReader::ReadFileBytes(input);
        const std::vector<u8> compressed = LZSS::CompressLzss10(file.data, file.size);
        FileWriter::WriteFile(output, compressed);
    }
}
