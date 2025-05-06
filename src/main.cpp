#include "Commands/CommandType.h"
#include "Commands/LZSSCommands.h"
#include "Commands/TPLCommands.h"
#include "Commands/U8Commands.h"
#include "Commands/debug_commands.h"
#include "Commands/MapCommands.h"
#include "core/Logging.h"
#include <cstring>

using namespace SPMEditor;

Command lzssCommands[] = {
    {
        .name = "decompress",
        .format = "<input file> <output file>",
        .description = "Decompresses an lzss compressed file.",
        .parameter_count = 2,
        .run = LZSSCommands::Decompress,
    }, {
        .name = "compress",
        .format = "<input file> <output file>",
        .description = "Uses lzss to compress a file.",
        .parameter_count = 2,
        .run = LZSSCommands::Compress,
    },
};

Command debugCommands[] = {
#ifndef SPME_NO_VIEWER
    {
        .name = "view",
        .format = "<map.bin>",
        .description = "Opens a 3d preview of a SPM map.bin file",
        .parameter_count = 1,
        .run = debug_command_view,
    },
#endif
};

Command mapCommands[] = {
    {
        .name = "to_fbx",
        .format = "<map.bin> <output file> [map name]",
        .description = "Converts a SPM map.bin file into an FBX file. The map file should be taken from DATA/files/map.",
        .parameter_count = 2,
        .run = MapCommands::ToFBX,
    }, {
        .name = "from_glb",
        .format = "<map.glb> <config.yaml path> <output_directory>",
        .description = "Converts a glb model to a SPM map.dat file. output_directory is a u8 extracted directory with existing map data (required for cameraroad.bin, setup.bin, and skybox textures). Please run u8 compile to create a spm map.bin to reinsert into the game files.",
        .parameter_count = 3,
        .run = MapCommands::FromGLB,
    },  {
        .name = "create_config",
        .format = "<map name> <map.glb> <output path>",
        .description = "Creates a map config from an existing model",
        .parameter_count = 3,
        .run = MapCommands::CreateConfig,
    }, 
};

Command tplCommands[] = {
    {
        .name = "dump",
        .format = "<texture.tpl> <output directory>",
        .description = "Writes all textures in a tpl to a directory",
        .parameter_count = 2,
        .run = TPLCommands::Dump,
    },
};
 
Command u8Commands[] = {
    {
        .name = "extract",
        .format = "<u8 file> <output directory> <compressed>",
        .description = "Extracts the contents of a u8 to a directory",
        .parameter_count = 2,
        .run = U8Commands::Extract,
    }, {
        .name = "compile",
        .format = "<directory> <output file>",
        .description = "Creates a u8 archive file from a directory",
        .parameter_count = 3,
        .run = U8Commands::Compile,
    },
};

CommandGroup commandGroups[] = {
    {
        .name = "u8",
        .commands = u8Commands,
        .command_count = sizeof(u8Commands) / sizeof(Command),
    }, {
        .name = "tpl",
        .commands = tplCommands,
        .command_count = sizeof(tplCommands) / sizeof(Command),
    }, {
        .name = "map",
        .commands = mapCommands,
        .command_count = sizeof(mapCommands) / sizeof(Command),
    }, {
        .name = "debug",
        .commands = debugCommands,
        .command_count = sizeof(debugCommands) / sizeof(Command),
    }, {
        .name = "lzss",
        .commands = lzssCommands,
        .command_count = sizeof(lzssCommands) / sizeof(Command),
    }
};

constexpr u32 commandGroupCount = sizeof(commandGroups) / sizeof(CommandGroup);

char CharToLower(char c);
void StringToLower(char* string);

bool FindCommandGroup(const char* name, CommandGroup** out_group);
void DisplayAvailableCommands();

int main(int argc, char** argv) {
    // Initialization
    LoggingInitialize();

    if (argc <= 2) {
        LogError("Invalid number of arguments");
        DisplayAvailableCommands();
        return -1;
    }

    // Search for the target command group
    char* target_group = argv[1];
    StringToLower(target_group);
    CommandGroup* target_command_group = nullptr;

    if (!FindCommandGroup(target_group, &target_command_group)) {
        DisplayAvailableCommands();
        return -1;
    }

    // Search for the command inside the group
    char* command_name = argv[2];
    StringToLower(command_name);

    bool found_command = false;
    const Command* command = nullptr;
    for (u32 i = 0; i < target_command_group->command_count; i++) {
        if (strcmp(target_command_group->commands[i].name, command_name) == 0) {
            found_command = true;
            command = &target_command_group->commands[i];
            LogDebug("Found command '%s'", command->name);
            break;
        }
    }

    if (!found_command) {
        LogError("Failed to find command '%s' in group '%s'."); 
        DisplayAvailableCommands();
        return -1; 
    }

    // Run the command if parameters are correct
    if ((u32)(argc - 3) < command->parameter_count) {
        LogError("Cannot run command '%s %s' because an invalid number of parameters were given. Expected %d parameters, got %d", 
                target_command_group->name, 
                command->name, 
                command->parameter_count, 
                argc - 3);
        DisplayAvailableCommands();
        return -1;
    }

    LogInfo("Running command %s %s", target_command_group->name, command->name);
    command->run(argc - 3, (const char**)argv + 3); // Cast argv to const

    // Shutdown
    LoggingShutdown();

    return 0;
}

bool FindCommandGroup(const char* target_group_name, CommandGroup** out_group) {
    bool foundCommandGroup = false;
    for (u32 i = 0; i < commandGroupCount; i++) {
        CommandGroup* group = &commandGroups[i];
        if (strcmp(group->name, target_group_name) == 0) {
            foundCommandGroup = true;
            *out_group = group;
            break;
        }
    }

    if (!foundCommandGroup) {
        LogError("Failed to find command group '%s'", target_group_name);
        return false;
    }

    return true;
}

char CharToLower(char c) {
    if (c >= 'A' && c <= 'Z') 
        return c + ('a' - 'A');
    return c;
}

void StringToLower(char* string) {
    u32 len = strlen(string);
    for (u32 i = 0; i < len; i ++) {
        string[i] = CharToLower(string[i]);
    }
}

void DisplayAvailableCommands() {
    LogInfo("Available commands are: ");
    for (u32 i = 0; i < commandGroupCount; i++) {
        LogInfo("\t%s", commandGroups[i].name);

        for (u32 c = 0; c < commandGroups[i].command_count; c++) {
            Command* command = &commandGroups[i].commands[c];
            LogInfo("\t\t%s", command->name);
            LogInfo("\t\tFormat:      %s", command->format);
            LogInfo("\t\tDescription: %s", command->description);
            LogInfo("");
        }

    }
}

