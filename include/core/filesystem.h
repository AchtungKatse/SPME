#pragma once
#include "IO/FileHandle.h"

namespace SPMEditor {

    bool filesystem_exists(const char* name);
    void filesystem_write_file(const char* path, const u8* data, u64 length);
    FileHandle filesystem_read_file(const char* path);
}
