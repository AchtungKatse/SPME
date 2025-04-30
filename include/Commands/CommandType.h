#pragma once

namespace SPMEditor {
    typedef struct {
        const char* name;
        const char* format;
        const char* description;
        const u32 parameter_count;
        void (*run)(u32 argc, const char** argv);
    } command_t;

    typedef struct {
        const char* name;
        command_t* commands;
        u32 command_count;
    } command_group_t;
}
