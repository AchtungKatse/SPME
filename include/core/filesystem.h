#pragma once

#include "defines.h"
#include "Types/Types.h"

SPME_HEADER_TOP

typedef struct {
    u8* data;
    u64 size;
} file_handle_t;

b8 filesystem_exists(const char* name);
void filesystem_write_file(const char* path, const u8* data, u64 length);
file_handle_t filesystem_read_file(const char* path);

SPME_HEADER_BOTTOM
