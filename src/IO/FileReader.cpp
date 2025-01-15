#include "FileReader.h"

namespace SPMEditor::FileReader {
    std::vector<u8> ReadFileBytes(string path)
    {
        Assert(std::filesystem::exists(path), "Failed to find file '{}'", path);

        ifstream file (path, ios::binary | ios::ate);
        int size = file.tellg();
        file.seekg(0, ios::beg);

        vector<u8> data(size);
        file.read((char*)data.data(), size);

        Assert(size > 0, "Error reading file '{}', invalid size '{}'", path, size);
        return data;
    }
}
