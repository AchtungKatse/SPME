#pragma once

#include "defines.h"
SPME_HEADER_TOP

u8* lzss_decompress(const u8* data, int length, int* out_decompressed_size);
u8* lzss_compress_10(u8* data, u64 size);

u8* lzss_decompress_10(const u8* indata, int compressedSize, u32* out_decompressedSize);
u8* lzss_decompress_11(const u8* indata, int compressedSize, u32* out_decompressedSize);

SPME_HEADER_BOTTOM
