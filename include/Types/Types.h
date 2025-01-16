#pragma once
#include "assimp/color4.h"
#include "assimp/vector3.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

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

typedef vec3<float> Vector3;

typedef vec2<float> Vector2;

inline u16 ByteSwap(u16 val)
{
    return __builtin_bswap16(val);
}
inline u32 ByteSwap(u32 val)
{
    return __builtin_bswap32(val);
}
inline short ByteSwap(short val)
{
    return __builtin_bswap16(val);
}
inline int ByteSwap(int val)
{
    return __builtin_bswap32(val);
}

inline void ByteSwap4(void* data, int elementCount)
{
    for (int i = 0; i < elementCount; i++) {
        ((int*)data)[i] = __builtin_bswap32(((int*)data)[i]);
    }
}

inline void ByteSwap2(void* data, int elementCount)
{
    for (int i = 0; i < elementCount; i++) {
        ((u16*)data)[i] = __builtin_bswap16(((u16*)data)[i]);
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
