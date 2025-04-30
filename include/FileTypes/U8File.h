#pragma once

SPME_HEADER_TOP

    typedef struct {
        const char* name;
        u8* data;
        u64 size;
    } U8File;

    typedef struct u8_directory {
            const char* name;
            U8File* files;
            struct u8_directory* subdirs;
            u32 subdir_count;
            u32 file_count;
    } u8_directory_t;

    bool u8_directory_get_file(const char* path, U8File** outFile);
    bool u8_directory_file_exists(const char* path);
    void u8_directory_add_file(const char* path, U8File file);
    void u8_directory_dump(const char* outputDir);
    int u8_directory_get_total_file_count();
    int u8_directory_get_total_node_count();
    int u8_directory_get_total_name_size();
    int u8_directory_get_total_file_size_padded();

SPME_HEADER_BOTTOM
