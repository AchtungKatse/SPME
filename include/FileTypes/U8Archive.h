#pragma once
#include "FileTypes/U8File.h"

SPME_HEADER_TOP

    // Thanks Wiibrew (https://wiibrew.org/wiki/U8_archive)
#define U8_ARCHIVE_FILE_MAGIC 0x55AA382D

typedef struct {
    u32 file_magic; // 0x55AA382D "U.8-"
    u32 rootOffset; // offset to root_node, always 0x20.
    u32 size; // size of header from root_node to end of string table.
    u32 dataOffset; // offset to data -- this is rootnode_offset + header_size, aligned to 0x40.
    u8 padding[16];
} u8_archive_header_t;

typedef struct {
    u16 type; //this is really a u8
    u16 nameOffset; //really a "u24"
    u32 dataOffset;
    u32 size;
} u8_archive_node_t;

typedef struct {
        u8_directory_t rootDirectory;
} U8Archive;

U8Archive u8_archive_read_from_file(const char* path, bool compressed);
U8Archive u8_archive_read_from_bytes(const u8* data, u32 size, bool compressed);
bool u8_archive_try_create_from_directory(const char* path, U8Archive* output);

bool u8_archive_file_exists(U8Archive* archive, const char* path);
bool u8_archive_get_file(U8Archive* archive, const char* path, U8File** outFile);

void u8_archive_dump(U8Archive* archive, const char* path);
u8* u8_archive_compile(U8Archive* archive, u32* out_archive_size);

SPME_HEADER_BOTTOM
