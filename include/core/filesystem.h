#pragma once

namespace SPMEditor {
    typedef struct {
        void* data;
        u64 size;
    } FileHandle;

    bool filesystem_exists(const char* name);
    void filesystem_write_file(const char* path, const u8* data, u64 length);
    FileHandle filesystem_read_file(const char* path);
}
