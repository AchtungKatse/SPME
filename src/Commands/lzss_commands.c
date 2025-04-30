#include "Commands/lzss_commands.h"
#include "defines.h"
#include "Compressors/LZSS.h"
#include "core/logging.h"
#include "core/filesystem.h"

SPME_HEADER_TOP

void lzss_command_compress(u32 argc, char** argv) {
    const char* input = argv[0];
    const char* output = argv[1];

    Assert(filesystem_exists(input), "File '%s' Does not exist.", input);

    file_handle_t file_handle = filesystem_read_file(input);
    u8* compressed_data = lzss_compress_10(file_handle.data, file_handle.size);
    filesystem_write_file(output, compressed_data, file_handle.size);
}

void lzss_command_decompress(u32 argc, char** argv) {
    const char* input = argv[0];
    const char* output = argv[1];

    Assert(filesystem_exists(input), "File '%s' Does not exist.", input);

    file_handle_t compressed = filesystem_read_file(input);

    int decompressed_size = 0;
    u8* decompressed = lzss_decompress(compressed.data, compressed.size, &decompressed_size);
    filesystem_write_file(output, decompressed, decompressed_size);
}

SPME_HEADER_BOTTOM
