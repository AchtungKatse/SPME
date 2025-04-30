#pragma once

typedef struct {
    const char* name;
    const char* format;
    const char* description;
    const int parameter_count;
    void (*run)(u32 argc, const char** args);
} command_t;

typedef struct {
    const char* name;
    const u32 command_count;
    const command_t* commands;
} command_group_t;
