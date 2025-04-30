#include "core/filesystem.h"
#include <filesystem>
#include <fstream>

namespace SPMEditor {
    bool filesystem_exists(const char* name) {
        return std::filesystem::exists(name);
    }

    void filesystem_write_file(const char* path, const u8* data, u64 length) {
        std::ofstream file(path, std::ios::out | std::ios::binary);
        file.write((char*)data, length);
    }

    FileHandle filesystem_read_file(const char* path) {
        Assert(std::filesystem::exists(path), "Failed to find file '%s'", path);

        std::ifstream file (path, std::ios::binary | std::ios::ate);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        u8* data = new u8[size];
        file.read((char*)data, size);

        FileHandle handle = {
            .data = data,
            .size = size,
        };

        Assert(size > 0, "Error reading file '%s', invalid size '%d'", path, size);
        return handle;
    }
}
