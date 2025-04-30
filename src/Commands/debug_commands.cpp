#include "Commands/debug_commands.h"
#include "core/filesystem.h"
#include "core/Logging.h"
#include "FileTypes/LevelData.h"
#include "Commands/Display/Display.h"

namespace SPMEditor {
    void debug_command_view(u32 argc, const char** argv) {
        const char* fileName = argv[0];
        Assert(filesystem_exists(fileName), "Debug view cannot open because file '%s' does not exist.");

        LevelData level = LevelData::LoadLevelFromFile(fileName, true);
        Display::DisplayLevel(level);
    }

}
