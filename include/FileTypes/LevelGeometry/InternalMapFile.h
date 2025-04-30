#pragma once
#include "FileTypes/TPL.h"
#include "Types/Types.h"
#include "assimp/vector3.h"
#include "defines.h"

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

SPME_HEADER_TOP

typedef enum : u32 {
    VERTEX_ATTRIBUTE_POSITION = 1,
    VERTEX_ATTRIBUTE_NORMAL = 2,
    VERTEX_ATTRIBUTE_COLOR = 4,
    VERTEX_ATTRIBUTE_UNK_1 = 8,
    VERTEX_ATTRIBUTE_UV = 0x10,
    VERTEX_ATTRIBUTE_UNK_2 = 0x20,
} VertexAttributes;

typedef struct {
    int entryOffset;
    int length;
} vertex_strip_header_t;

PACK(struct VertexStrip {
        u8 unknown;
        u16 vertexCount;
        });

typedef struct {
    int constant;
    int entryCount;
    VertexAttributes vertexAttributes;
    int VCDOffset;
} MeshHeader;

typedef struct {
    int fileLength;
    int pointerListStart;
    int pointerEntryCount;
    int sectionCount;
    u8 padding[0x10];
} FileHeader;

typedef struct {
    int fileOffset;
    const char* name;
} Section;

typedef struct {
    char* version;
    int objHeirarchyOffset;
    char* rootObjName;
    char* rootColliderName;
    char* timestamp;
} InfoSection;

typedef struct {
    int version;
    int objHeirarchyOffset;
    int rootObjName;
    int rootColliderName;
    int timestamp;
} RawInfoSection;

typedef struct {
    u8 data[0x14];
} object_subdata_t;

typedef struct {
    int name; // char*
    int type; // char*
              
    int parent; // RawObject*
    int child; // RawObject*
    int nextSibling; // RawObject*
    int previousSibling; // RawObject*
                         
    vec3 scale;
    vec3 rotation; 
    vec3 position;

    vec3 boundsMin;
    vec3 boundsMax;

    int padding;
    int subdataPtr; // Guesstimating this datastructure 0x14 bytes long or a multiple of 0x14, Seems to somehow affect rendering / transparency mode. On everything but root object
    int meshCount;
    // then for each mesh in mesh count (with a minimum of 1)
    // Material* material;
    // Mesh* mesh;
    // If meshCount == 0 then the first material* and mesh* are 0
} RawObject;

typedef struct {
    int vertexOffset;
    int normalOffset;
    int unknown_1;
    int colorOffset;
    int unknown_2;
    int unknown_3;
    int uvOffset;
    int unknown_4[10];
    int vertexScale;
    int uvScale;
} RawVCDTable;

typedef struct 
{
    int vertexCount;
    int normalCount;
    int colorCount;
    int uvCount;
    int unknown[13];
    int vertexScale;
    int uvScale;

    vec3_u16* vertices;
    Color* normals;
    Color* colors;
    vec2_u16* uvs;
} VCDTable;

// Used in material_name_table
typedef struct 
{
    int nameOffset; // pointer to name
    int materialOffset; // Pointer to Material
} MaterialNameEntry;

typedef struct {
    vec2 unk_1;
    vec2 value;
    vec2 unk_2;
    float unk_3;
} material_parameter_t;

typedef struct {
    u8 unkown[0xc];
} material_subdata_t;

typedef struct {
    int nameOffset; // Really a char* but its similar enough
    Color color;
    bool useVertexColor;
    bool unk_1;
    bool useTransparency;
    bool useTexture;
    int textureInfoPtr; // From my old notes. Idfk what this is
                        // The structure is 0x114 bytes long but idk what the rest of everything is atm so I'm just filling it with unknown floats (most of the values are floats)
    material_parameter_t unk_param_1;
    material_parameter_t uvScale;
    material_parameter_t unk_param_3;
    material_parameter_t unk_param_4;
    material_parameter_t unk_param_5;
    material_parameter_t unk_param_6;
    material_parameter_t unk_param_7;
    material_parameter_t unk_param_8;
    material_parameter_t unk_param_9;
    Color color2; // Could be something else
    int subdataOffset;
} Material;

typedef struct 
{
    u32 dataOffset;
    u32 unknown; // Padding?
    u8 wrapU; // NOTE: Same as TPL Image wrap mode but u8 instead of u32
    u8 wrapV;
    u8 unk_3;
    u8 unk_4;
} texture_info_;

    typedef enum : u8 {
        Opaque = 0,
        Clip = 1,
    } texture_transparency_type_t;

// Used exclusively in the map.dat file to describe a texture in textures.tpl
typedef struct {
    u32 nameOffset;
    texture_transparency_type_t transparency;
    u8 unk_2;
    u8 unk_3;
    u8 unk_4;
    u16 width;
    u16 height;
    u32 unk_5;
} MapTexture;

typedef struct {
    float start;
    float end;
    // Colors might be near / far colors used for blending
    Color color;
    Color unknown; 
} FogEntry;

typedef struct {
    float start;
    float unknown;
    float increment; // Added every frame
    float unknown_2;
    float padding;
} PropertyCurve;

typedef struct {
    PropertyCurve x, y; 
} PropertyCurve2D;

typedef struct {
    PropertyCurve x, y, z;
} PropertyCurve3D;

typedef struct {
    float startFrame;
    PropertyCurve3D position, rotation, scale;
    float unknown[0x3C];
} transform_animation_keyframe_t;

typedef struct {
    int nameOffset;
    float unknown[6];
    vec3 baseScale;
    vec3 basePosition;
    vec3 baseRotation;
    float unkown_2[6];
    u32 keyframeCount;
} transform_animation_t;

typedef struct {
    float startFrame;
    PropertyCurve2D offset, scale;
    float unknown[5];
} material_animation_keyframe_t;

typedef struct { 
    int materialNameOffset;
    float unknown[3];
    int keyframeCount;
    // Followed by
    // Keyframe keyframes[keyframeCount];
} material_animation_t;

typedef struct {
        const char* animationName;
        float unknown[3];
        material_animation_keyframe_t* keyframes;
} MaterialAnimation;

typedef struct {
    int nameOffset;
    int constant;
    float frameCount;
    int transformAnimationOffset;
    int materialAnimationOffset;
    int padding[4];
} AnimationHeader;

typedef struct {
    const char* name;
    float frameCount;
} Animation;

SPME_HEADER_BOTTOM
