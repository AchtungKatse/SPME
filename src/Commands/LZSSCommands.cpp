#include "Commands/LZSSCommands.h"
#include "IO/FileHandle.h"
#include "core/Logging.h"
#include "core/filesystem.h"
#include "Compressors/LZSS.h"

namespace SPMEditor {

    void lzss_command_compress(u32 argc, char** argv) {
        const char* input = argv[0];
        const char* output = argv[1];

        Assert(filesystem_exists(input), "File '%s' Does not exist.", input);

        FileHandle file_handle = filesystem_read_file(input);
        std::vector<u8> compressed_data = LZSS::CompressLzss10(file_handle.data, file_handle.size);
        filesystem_write_file(output, compressed_data.data(), file_handle.size);
    }

    void lzss_command_decompress(u32 argc, char** argv) {
        const char* input = argv[0];
        const char* output = argv[1];

        Assert(filesystem_exists(input), "File '%s' Does not exist.", input);

        FileHandle compressed = filesystem_read_file(input);

        int decompressed_size = 0;
        std::vector<u8> decompressed = LZSS::DecompressBytes(compressed.data, compressed.size);
        filesystem_write_file(output, decompressed.data(), decompressed_size);
    }

}
