#pragma once
#include "assimp/texture.h"
#include "defines.h"

SPME_HEADER_TOP

typedef enum : u32 {
    I4 = 0,
    I8 = 1,
    IA4 = 2,
    IA8 = 3,
    RGB565 = 4,
    RGB5A3 = 5,
    RGBA32 = 6,
    C4 = 8,
    C8 = 9,
    C14X2 = 0xA,
    CMPR = 0xE,
} tpl_image_format_t;

typedef struct {
    struct aiTexture* mTexture;
    tpl_image_format_t mFormat;
} tpl_image_create_info_t;

typedef struct {
    tpl_image_create_info_t* mImageCreateInfos;
    uint mImageCreateInfoCount;
} tpl_create_info_t;

typedef enum : u32 {
    Clamp = 0,
    Repeat = 1,
    Mirror = 2,
} tpl_image_wrap_mode_t;

typedef enum : u32 { 
    TPL_PALETTE_IA8 = 0, 
    TPL_PALETTE_RGB565 = 1, 
    TPL_PALETTE_RGB5A3 = 2,
} tpl_palette_format_t;

typedef struct {
    u16 entryCount;
    u8 unpacked;
    u8 padding;
    tpl_palette_format_t format;
    u32 dataOffset;
} tpl_palette_header_t;


typedef struct
{
    u16	height;
    u16	width;
    tpl_image_format_t format;
    u32	imageDataAddress;
    tpl_image_wrap_mode_t wrapS;
    tpl_image_wrap_mode_t wrapT;
    u32	minFilter;
    u32	magFilter;
    float LODBias;
    u8	edgeLODEnable;
    u8	minLOD;
    u8	maxLOD;
    u8	padding;
} tpl_image_header_t;

typedef struct 
{
    tpl_image_header_t header;
    const char* name;
    Color* pixels;
} tpl_image_t;

typedef struct {
    int magic;
    int numImages;
    int imageTableOffset;
} tpl_header_t;

typedef struct {
    int headerOffset;
    int paletteOffset;
} image_offset_t;

typedef struct {
    tpl_image_t* images;
    u32 image_count;
} tpl_t;

tpl_t tpl_load_from_file(const char* path);
tpl_t tpl_load_from_memory(const u8* data, u64 size);
tpl_t tpl_create(const tpl_create_info_t* info);

void tpl_write(const char* path);

SPME_HEADER_BOTTOM
