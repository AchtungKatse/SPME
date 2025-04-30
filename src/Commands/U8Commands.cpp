#include "Commands/U8Commands.h"
#include "FileTypes/U8Archive.h"
#include "Compressors/LZSS.h"
#include "core/filesystem.h"
#include <cstring>
#include <filesystem>
#include <vector>

SPME_HEADER_TOP

void u8_command_compile(u32 argc, const char** argv) {
    // Get parameters
    const char* input = argv[0];
    const char* output = argv[1];
    const char* compressed = argv[2];

    // Validate input
    Assert(std::filesystem::exists(input), "Directory '%s' does not exist.", input);

    // Load the archive from file
    U8Archive archive;
    bool created_u8 = u8_archive_try_create_from_directory(input, &archive);
    Assert(created_u8, "Failed to create U8 archive from directory '%s'", input);

    u32 archive_size = 0;
    const u8* data = u8_archive_compile(&archive, &archive_size);

    // Decompress the archive if needed
    if (strcmp(compressed, "1") == 0 || strcmp(compressed, "true") == 0) {
        // NOTE: this copies the archive_size to lzss_decompress_10 then overwrites it for the decompressed size
        data = lzss_decompress_10(data, archive_size, &archive_size);
    }

    filesystem_write_file(output, data, archive_size);
}

void u8_command_extract(u32 argc, const char** argv) {
    // Get parameters
    const char* input = argv[0];
    const char* output = argv[1];

    // Read the archive from file
    Assert(std::filesystem::exists(input), "u8_command_compile failed. File '%s' does not exist.", input);
    U8Archive archive = u8_archive_read_from_file(input, true);

    // Dump it to the output directory
    u8_archive_dump(&archive, output);
}

SPME_HEADER_BOTTOM
