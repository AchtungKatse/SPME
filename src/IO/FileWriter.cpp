#include "IO/FileWriter.h"
#include <fstream>

namespace SPMEditor
{
    void FileWriter::WriteFile(const std::string& path, const std::vector<u8>& data)
    {
        WriteFile(path, data.data(), data.size());
    }

    void FileWriter::WriteFile(const std::string& path, const u8* data, int length)
    {
        std::ofstream file(path, std::ios::out | std::ios::binary);
        file.write((char*)data, length);
    }
}
