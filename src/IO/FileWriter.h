#pragma once
#include "PCH.h"

namespace SPMEditor::FileWriter {
    void WriteFile(string path, vector<u8> data);
    void WriteFile(string path, u8* data, int length);
}
