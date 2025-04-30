#include "Commands/LZSSCommands.h"
#include "core/Logging.h"
#include "core/filesystem.h"
#include "Compressors/LZSS.h"

namespace SPMEditor {

    void lzss_command_compress(u32 argc, const char** argv) {
        const char* input = argv[0];
        const char* output = argv[1];

        Assert(filesystem_exists(input), "File '%s' Does not exist.", input);

        FileHandle file_handle = filesystem_read_file(input);
        std::vector<u8> compressed_data = LZSS::CompressLzss10((u8*)file_handle.data, file_handle.size);
        filesystem_write_file(output, compressed_data.data(), file_handle.size);
    }

    void lzss_command_decompress(u32 argc, const char** argv) {
        const char* input = argv[0];
        const char* output = argv[1];

        Assert(filesystem_exists(input), "File '%s' Does not exist.", input);

        FileHandle compressed = filesystem_read_file(input);

        std::vector<u8> decompressed = LZSS::DecompressBytes((u8*)compressed.data, compressed.size);
        LogInfo("Writing 0x%x bytes of decompressed data", decompressed.size());
        filesystem_write_file(output, decompressed.data(), decompressed.size());
    }

}
