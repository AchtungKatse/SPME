#pragma once
#include "IO/FileHandle.h"

namespace SPMEditor::FileReader {
    FileHandle ReadFileBytes(const std::string& path);
}
