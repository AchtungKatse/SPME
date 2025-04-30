#include "Commands/debug_commands.h"
#include "FileTypes/LevelData.h"
#include "Commands/Display/Display.h"
#include <filesystem>

using namespace SPMEditor;

void debug_command_view(u32 argc, const char** argv) {
    const char* fileName = argv[0];
    Assert(std::filesystem::exists(fileName), "Debug view cannot run because SPME cannot find file '%s'", fileName);
    Assert(std::filesystem::is_regular_file(fileName), "Debug view cannot run because file '%s' is not a regular file", fileName);

    LevelData level = LevelData::LoadLevelFromFile(fileName, true);
    Display::DisplayLevel(level);
}
