#pragma once
#include "assimp/color4.h"
#include "assimp/vector3.h"
#include <stdint.h>

typedef unsigned char u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef int16_t s64;

typedef unsigned int uint;

struct Color 
{
    Color() = default;
    Color(u8 r, u8 g, u8 b, u8 a)
    {
        this->r = r;
        this->g = g;
        this->b = b;
        this->a = a;
    }

    u8 r;
    u8 g;
    u8 b;
    u8 a;

    operator aiColor4D() {return aiColor4D(r, g, b, a); }
};

template<typename T>
struct vec3
{
    vec3() = default;
    vec3(T x, T y, T z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    T x,y,z;

    template<typename Other>
        explicit operator vec3<Other>() {return vec3<Other>((Other)x, (Other)y, (Other)z); }

    operator aiVector3t<float>() {return aiVector3t<float>((float)x, (float)y, (float)z); }
};

template<typename T>
struct vec2
{
    vec2() = default;
    vec2(T x, T y)
    {
        this->x = x;
        this->y = y;
    }

    T x,y;

    template<typename Other>
        explicit operator vec2<Other>() {return vec2<Other>((Other)x, (Other)y); }
    operator aiVector3t<float>() {return aiVector3t<float>((float)x, (float)y, 0); }
};

typedef vec3<int> Vector3i;
typedef vec2<int> Vector2i;
typedef vec3<float> Vector3;
typedef vec2<float> Vector2;

inline short ByteSwap(short val)
{
    char* bytes = (char*)&val;
    char newData[2];

    newData[0] = bytes[1];
    newData[1] = bytes[0];
    return *(short*)newData;
}
inline float ByteSwap(float val)
{
    char* bytes = (char*)&val;
    char newData[4];

    newData[3] = bytes[0];
    newData[2] = bytes[1];
    newData[1] = bytes[2];
    newData[0] = bytes[3];
    return *(float*)newData;
}
inline int ByteSwap(int val) {
    char* bytes = (char*)&val;
    char newData[4];

    newData[0] = bytes[3];
    newData[1] = bytes[2];
    newData[2] = bytes[1];
    newData[3] = bytes[0];
    return *(int*)newData;
}
inline u8 ByteSwap(u8 val) { return val; }
inline s8 ByteSwap(s8 val) { return val; }
inline u16 ByteSwap(u16 val) { return ByteSwap((short)val); }
inline u32 ByteSwap(u32 val) { return ByteSwap((int)val); }

inline void ByteSwap4(void* data, int elementCount)
{
    for (int i = 0; i < elementCount; i++) {
        ((int*)data)[i] = ByteSwap(((int*)data)[i]);
    }
}

inline void ByteSwap2(void* data, int elementCount)
{
    for (int i = 0; i < elementCount; i++) {
        ((u16*)data)[i] = ByteSwap(((u16*)data)[i]);
    }
}

inline void ByteSwap(int* data, int elementCount)
{
    for (int i = 0; i < elementCount; i++) {
        data[i] = ByteSwap(data[i]);
    }
}

inline void ByteSwap(short* data, int elementCount)
{
    for (int i = 0; i < elementCount; i++) {
        data[i] = ByteSwap(data[i]);
    }
}
