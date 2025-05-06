#include "Commands/U8Commands.h"
#include "FileTypes/U8Archive.h"
#include "Compressors/LZSS.h"
#include "core/filesystem.h"
#include <cstring>
#include <filesystem>
#include <vector>

namespace SPMEditor::U8Commands {

    void Compile(u32 argc, const char** argv) {
        // Get parameters
        const char* input = argv[0];
        const char* output = argv[1];
        const char* compressed = argv[2];

        // Validate input
        Assert(std::filesystem::exists(input), "Directory '%s' does not exist.", input);

        // Load the archive from file
        U8Archive archive;
        bool created_u8 = U8Archive::TryCreateFromDirectory(input, archive);
        Assert(created_u8, "Failed to create U8 archive from directory '%s'", input);

        std::vector<u8> data = archive.CompileU8();

        // Decompress the archive if needed
        if (strcmp(compressed, "1") == 0 || strcmp(compressed, "true") == 0) {
            // NOTE: this copies the archive_size to lzss_decompress_10 then overwrites it for the decompressed size
            data = LZSS::CompressLzss10(data.data(), data.size());
        }

        filesystem_write_file(output, data.data(), data.size());
    }

    void Extract(u32 argc, const char** argv) {
        // Get parameters
        const char* input = argv[0];
        const char* output = argv[1];

        // Read the archive from file
        Assert(std::filesystem::exists(input), "u8_command_compile failed. File '%s' does not exist.", input);
        U8Archive archive = U8Archive::ReadFromFile(input, true);

        // Dump it to the output directory
        archive.Dump(output);
    }

}
