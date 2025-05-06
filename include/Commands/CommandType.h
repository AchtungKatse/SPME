#pragma once

namespace SPMEditor {
    typedef struct {
        const char* name;
        const char* format;
        const char* description;
        const u32 parameter_count;
        void (*run)(u32 argc, const char** argv);
    } Command;

    typedef struct {
        const char* name;
        Command* commands;
        u32 command_count;
    } CommandGroup;
}
