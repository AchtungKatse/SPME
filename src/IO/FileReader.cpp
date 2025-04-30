#include "IO/FileReader.h"
#include <fstream>

namespace SPMEditor {
    FileHandle FileReader::ReadFileBytes(const std::string& path)
    {
        Assert(std::filesystem::exists(path), "Failed to find file '%s'", path.c_str());

        std::ifstream file (path, std::ios::binary | std::ios::ate);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        u8* data = new u8[size];
        file.read((char*)data, size);

        FileHandle handle = {
            .data = data,
            .size = size,
        };

        Assert(size > 0, "Error reading file '%s', invalid size '%d'", path.c_str(), size);
        return handle;
    }
}
