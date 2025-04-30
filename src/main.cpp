#include "Commands/CommandType.h"
#include "Commands/LZSSCommands.h"
#include "Commands/TPLCommands.h"
#include "Commands/U8Commands.h"
#include "Commands/debug_commands.h"
#include "Commands/MapCommands.h"
#include "core/Logging.h"
#include <cstring>

using namespace SPMEditor;

command_t lzss_commands[] = {
    {
        .name = "decompress",
        .format = "<input file> <output file>",
        .description = "Decompresses an lzss compressed file.",
        .parameter_count = 2,
        .run = lzss_command_decompress,
    }, {
        .name = "compress",
        .format = "<input file> <output file>",
        .description = "Uses lzss to compress a file.",
        .parameter_count = 2,
        .run = lzss_command_compress,
    },
};

command_t debug_commands[] = {
    {
        .name = "view",
        .format = "<map.bin>",
        .description = "Opens a 3d preview of a SPM map.bin file",
        .parameter_count = 1,
        .run = debug_command_view,
    },
};

command_t map_commands[] = {
    {
        .name = "to_fbx",
        .format = "<map.bin> <output file> [map name]",
        .description = "Converts a SPM map.bin file into an FBX file. The map file should be taken from DATA/files/map.",
        .parameter_count = 2,
        .run = map_command_to_fbx,
    }, {
        .name = "from_glb",
        .format = "<map.glb> <output_file>",
        .description = "Converts a glb model to a SPM map.dat file. Note, this only changes the map.dat from a u8 extracted map file at [map_name]/dvd/map/[map_name]/map.dat. Please run u8 compile to create a spm map.bin to reinsert into the game files.",
        .parameter_count = 2,
        .run = map_command_from_glb,
    },
};

command_t tpl_commands[] = {
    {
        .name = "dump",
        .format = "<texture.tpl> <output directory>",
        .description = "Writes all textures in a tpl to a directory",
        .parameter_count = 2,
        .run = tpl_command_dump,
    },
};
 
command_t u8_commands[] = {
    {
        .name = "extract",
        .format = "<u8 file> <output directory> <compressed>",
        .description = "Extracts the contents of a u8 to a directory",
        .parameter_count = 2,
        .run = u8_command_extract,
    }, {
        .name = "compile",
        .format = "<directory> <output file>",
        .description = "Creates a u8 archive file from a directory",
        .parameter_count = 3,
        .run = u8_command_compile,
    },
};

command_group_t command_groups[] = {
    {
        .name = "u8",
        .commands = u8_commands,
        .command_count = sizeof(u8_commands) / sizeof(command_t),
    }, {
        .name = "tpl",
        .commands = tpl_commands,
        .command_count = sizeof(tpl_commands) / sizeof(command_t),
    }, {
        .name = "map",
        .commands = map_commands,
        .command_count = sizeof(map_commands) / sizeof(command_t),
    }, {
        .name = "debug",
        .commands = debug_commands,
        .command_count = sizeof(debug_commands) / sizeof(command_t),
    }, {
        .name = "lzss",
        .commands = lzss_commands,
        .command_count = sizeof(lzss_commands) / sizeof(command_t),
    }
};

constexpr u32 command_group_count = sizeof(command_groups) / sizeof(command_group_t);

char char_to_lower(char c);
void string_to_lower(char* string);

bool find_command_group(const char* name, command_group_t** out_group);
void display_available_commands();

int main(int argc, char** argv) {
    // Initialization
    LoggingInitialize();

    if (argc <= 2) {
        LogError("Invalid number of arguments");
        display_available_commands();
        return -1;
    }

    // Search for the target command group
    char* target_group = argv[1];
    string_to_lower(target_group);
    command_group_t* target_command_group = nullptr;

    if (!find_command_group(target_group, &target_command_group)) {
        display_available_commands();
        return -1;
    }

    // Search for the command inside the group
    char* command_name = argv[2];
    string_to_lower(command_name);

    bool found_command = false;
    const command_t* command = nullptr;
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
        display_available_commands();
        return -1; 
    }

    // Run the command if parameters are correct
    if ((u32)(argc - 3) < command->parameter_count) {
        LogError("Cannot run command '%s %s' because an invalid number of parameters were given. Expected %d parameters, got %d", 
                target_command_group->name, 
                command->name, 
                command->parameter_count, 
                argc - 3);
        display_available_commands();
        return -1;
    }

    LogInfo("Running command %s %s", target_command_group->name, command->name);
    command->run(argc - 3, (const char**)argv + 3); // Cast argv to const

    // Shutdown
    LoggingShutdown();

    return 0;
}

bool find_command_group(const char* target_group_name, command_group_t** out_group) {
    bool foundCommandGroup = false;
    for (u32 i = 0; i < command_group_count; i++) {
        command_group_t* group = &command_groups[i];
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

char char_to_lower(char c) {
    if (c >= 'A' && c <= 'Z') 
        return c + ('a' - 'A');
    return c;
}

void string_to_lower(char* string) {
    u32 len = strlen(string);
    for (u32 i = 0; i < len; i ++) {
        string[i] = char_to_lower(string[i]);
    }
}

void display_available_commands() {
    LogInfo("Available commands are: ");
    for (u32 i = 0; i < command_group_count; i++) {
        LogInfo("\t%s", command_groups[i].name);

        for (u32 c = 0; c < command_groups[i].command_count; c++) {
            command_t* command = &command_groups[i].commands[c];
            LogInfo("\t\t%s", command->name);
            LogInfo("\t\tFormat:      %s", command->format);
            LogInfo("\t\tDescription: %s", command->description);
            LogInfo("");
        }

    }
}

