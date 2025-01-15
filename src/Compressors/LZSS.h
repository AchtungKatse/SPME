#pragma once
#include "Types/Types.h"
#include <vector>

using namespace std;

namespace SPMEditor::LZSS
{
    vector<u8> DecompressBytes(vector<u8> input);
    vector<u8> DecompressBytes(u8* data, int length);
    vector<u8> CompressLzss11(vector<u8> data);
}
