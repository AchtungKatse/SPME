#include "IO/FileReader.h"

namespace SPMEditor {
    std::vector<u8> FileReader::ReadFileBytes(const std::string& path)
    {
        Assert(std::filesystem::exists(path), "Failed to find file '{}'", path);

        std::ifstream file (path, std::ios::binary | std::ios::ate);
        int size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<u8> data(size);
        file.read((char*)data.data(), size);

        Assert(size > 0, "Error reading file '{}', invalid size '{}'", path, size);
        return data;
    }
}
