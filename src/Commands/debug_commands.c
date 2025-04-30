#include "Commands/debug_commands.h"
#include "core/filesystem.h"
#include "core/logging.h"
#include "FileTypes/LevelData.h"
#include "defines.h"

SPME_HEADER_TOP

void debug_command_view(u32 argc, const char** argv) {
    const char* fileName = argv[0];
    Assert(filesystem_exists(fileName), "Debug view cannot open because file '%s' does not exist.");

    LevelData level = LevelData::LoadLevelFromFile(argv[i + 1], true);
    Display::DisplayLevel(level);
}

SPME_HEADER_BOTTOM
