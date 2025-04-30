#pragma once
#include "FileTypes/LevelGeometry/InternalMapFile.h"
#include "FileTypes/U8Archive.h"
#include "assimp/scene.h"

SPME_HEADER_TOP

typedef struct {
    U8Archive u8Files;
    char* name;
    aiScene* geometry;
    FogEntry* fogSettings;
    u32 fog_settings_count;
} LevelData;

LevelData level_data_load_from_file(const char* path, bool compressed, const char* mapNameOverride);
LevelData level_data_load_from_bytes(const char* name, const u8* data, u64 size, bool compressed, const std::string& mapNameOverride);

SPME_HEADER_BOTTOM
