#pragma once
#include <stdint.h>

typedef unsigned char b8;
typedef uint32_t b32;

typedef unsigned char u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef int16_t s64;

typedef unsigned int uint;

typedef struct {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} Color;

#define vec4(type, name) \
typedef struct {                \
    type x,y,z, w;              \
} name;                         \

#define vec3(type, name) \
typedef struct {                \
    type x,y,z;                 \
} name;                         \

#define vec2(type, name) \
typedef struct {                \
    type x,y;                   \
} name;                         \

vec4(float, vec4);
vec3(int, vec3i);
vec3(float, vec3);
vec3(u16, vec3_u16);
vec2(int, vec2i);
vec2(float, vec2);
vec2(u16, vec2_u16);

inline short byte_swap_short(short val)
{
    char* bytes = (char*)&val;
    char newData[2];

    newData[0] = bytes[1];
    newData[1] = bytes[0];
    return *(short*)newData;
}
inline float byte_swap_float(float val)
{
    char* bytes = (char*)&val;
    char newData[4];

    newData[3] = bytes[0];
    newData[2] = bytes[1];
    newData[1] = bytes[2];
    newData[0] = bytes[3];
    return *(float*)newData;
}
inline int byte_swap_int(int val) {
    char* bytes = (char*)&val;
    char newData[4];

    newData[0] = bytes[3];
    newData[1] = bytes[2];
    newData[2] = bytes[1];
    newData[3] = bytes[0];
    return *(int*)newData;
}

inline u16 byte_swap_u16(u16 val) { return byte_swap_short((short)val); }
inline u32 byte_swap_u32(u32 val) { return byte_swap_int((int)val); }

inline void byte_swap_array_stride_4(void* data, int elementCount) {
    for (int i = 0; i < elementCount; i++) {
        ((int*)data)[i] = byte_swap_int(((int*)data)[i]);
    }
}

inline void byte_swap_array_stride_2(void* data, int elementCount) {
    for (int i = 0; i < elementCount; i++) {
        ((u16*)data)[i] = byte_swap_u16(((u16*)data)[i]);
    }
}
