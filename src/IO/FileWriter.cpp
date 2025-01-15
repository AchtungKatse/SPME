#include "FileWriter.h"

namespace SPMEditor::FileWriter
{
    void WriteFile(string path, vector<u8> data)
    {
        WriteFile(path, data.data(), data.size());
    }

    void WriteFile(string path, u8* data, int length)
    {
        ofstream file(path, ios::out | ios::binary);
        file.write((char*)data, length);
    }
}
