#pragma once
#include "FileTypes/TPL.h"
#include "Types/Types.h"
#include "assimp/vector3.h"
#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

namespace SPMEditor::LevelInternal
{
    enum VertexAttributes : u32 {
        VERTEX_ATTRIBUTE_POSITION = 1,
        VERTEX_ATTRIBUTE_NORMAL = 2,
        VERTEX_ATTRIBUTE_COLOR = 4,
        VERTEX_ATTRIBUTE_UNK_1 = 8,
        VERTEX_ATTRIBUTE_UV = 0x10,
        VERTEX_ATTRIBUTE_UNK_2 = 0x20,
    };

    PACK(struct VertexStrip
    {
        struct Header
        {
            int entryOffset;
            int length;
        };

        u8 unknown;
        u16 vertexCount;
    });

    struct MeshHeader
    {
        int constant;
        int entryCount;
        VertexAttributes vertexAttributes;
        int VCDOffset;
    };

    struct FileHeader
    {
        int fileLength;
        int pointerListStart;
        int pointerEntryCount;
        int sectionCount;
        u8 padding[0x10];

        void ByteSwap();
    };

    struct Section
    {
        int fileOffset;
        std::string name;
    };

    struct InfoSection
    {
        char* version;
        int objHeirarchyOffset;
        char* rootObjName;
        char* rootColliderName;
        char* timestamp;
    };

    struct RawInfoSection
    {
        int version;
        int objHeirarchyOffset;
        int rootObjName;
        int rootColliderName;
        int timestamp;
    };

    struct RawObject
    {
        struct SubData {
            u8 data[0x14];
        };

        int name; // char*
        int type; // char*
        int parent; // RawObject*
        int child; // RawObject*
        int nextSibling; // RawObject*
        int previousSibling; // RawObject*

        Vector3 scale;
        Vector3 rotation; 
        Vector3 position;

        Vector3 boundsMin;
        Vector3 boundsMax;

        int padding;
        int subdataPtr; // Guesstimating this datastructure 0x14 bytes long or a multiple of 0x14, Seems to somehow affect rendering / transparency mode. On everything but root object
        int meshCount;

        // then for each mesh in mesh count (with a minimum of 1)
        // Material* material;
        // Mesh* mesh;
        // If meshCount == 0 then the first material* and mesh* are 0
    };

    struct RawVCDTable {
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

    };

    struct VCDTable
    {
        int vertexCount;
        int normalCount;
        int colorCount;
        int uvCount;
        int unknown[13];
        int vertexScale;
        int uvScale;

        vec3<short>* vertices;
        Color* normals;
        Color* colors;
        vec2<u16>* uvs;
    };

    struct MaterialNameEntry // Used in material_name_table
    {
        int nameOffset; // pointer to name
        int materialOffset; // Pointer to Material
    };

    struct Material
    {
        struct Parameter {
            Vector2 unk_1;
            Vector2 value;
            Vector2 unk_2;
            float unk_3;
        };

        struct SubData {
            u8 unkown[0xc];
        };

        int nameOffset; // Really a char* but its similar enough
        Color color;
        bool useVertexColor;
        bool unk_1;
        bool useTransparency;
        bool useTexture;
        int textureInfoPtr; // From my old notes. Idfk what this is
        // The structure is 0x114 bytes long but idk what the rest of everything is atm so I'm just filling it with unknown floats (most of the values are floats)
        Parameter unk_param_1;
        Parameter uvScale;
        Parameter unk_param_3;
        Parameter unk_param_4;
        Parameter unk_param_5;
        Parameter unk_param_6;
        Parameter unk_param_7;
        Parameter unk_param_8;
        Parameter unk_param_9;
        Color color2; // Could be something else
        int subdataOffset;
    };

    struct MapTexture // Used exclusively in the map.dat file to describe a texture in textures.tpl
    {
        enum class TransparencyType : u8 {
            Opaque = 0,
            Clip = 1,
        };

        struct Info
        {
            u32 dataOffset;
            u32 unknown; // Padding?
            u8 wrapU; // NOTE: Same as TPL Image wrap mode but u8 instead of u32
            u8 wrapV;
            u8 unk_3;
            u8 unk_4;
        };

        u32 nameOffset;
        TransparencyType transparency;
        u8 unk_2;
        u8 unk_3;
        u8 unk_4;
        u16 width;
        u16 height;
        u32 unk_5;
    };

    struct FogEntry {
        float start;
        float end;

        // Colors might be near / far colors used for blending
        Color color;
        Color unknown; 
    };

    struct PropertyCurve {
        float start;
        float unknown;
        float increment; // Added every frame
        float unknown_2;
        float padding;
    };

    struct PropertyCurve2D {
        PropertyCurve x, y; 
    };

    struct PropertyCurve3D {
        PropertyCurve x, y, z;
    };

    struct TransformAnimation {
        struct Keyframe {
            float startFrame;
            PropertyCurve3D position, rotation, scale;
            float unknown[0x3C];
        };

        int nameOffset;
        float unknown[6];
        aiVector3D baseScale;
        aiVector3D basePosition;
        aiVector3D baseRotation;
        float unkown_2[6];
        u32 keyframeCount;
    };

    class InternalMaterialAnimation { 
        public:
            struct Keyframe {
                float startFrame;
                PropertyCurve2D offset, scale;
                float unknown[5];
            };

            int materialNameOffset;
            float unknown[3];
            int keyframeCount;
            // Followed by
            // Keyframe keyframes[keyframeCount];
    };

    class MaterialAnimation {
        public:
            const char* animationName;
            float unknown[3];
            std::vector<InternalMaterialAnimation::Keyframe> keyframes;
    };

    struct AnimationHeader {
        int nameOffset;
        int constant;
        float frameCount;
        int transformAnimationOffset;
        int materialAnimationOffset;
        int padding[4];
    };

    struct Animation {
        const char* name;
        float frameCount;
    };
}
