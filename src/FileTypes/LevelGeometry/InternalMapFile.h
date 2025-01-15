#pragma once
namespace SPMEditor::LevelInternal
{
    enum struct VertexAttributes : u32
    {
        Position = 1,
        Unk_1 = 2,
        Color = 4,
        Unk_2 = 8,
        UV = 0x10
    };

    struct VertexStrip
    {
        struct Header
        {
            int entryOffset;
            int length;
        };

        u8 unknown;
        u16 vertexCount;
    }__attribute__((packed)); // Packing adds an extra u8 after unknown which breaks the vertex count

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
        int fatPointer;
        int fatEntryCount;
        int sectionCount;
        u8 padding[0x10];

        void ByteSwap();
    };

    struct Section
    {
        int fileOffset;
        string name;
    };

    struct InfoHeader
    {
        char* version;
        int objHeirarchyOffset;
        char* rootObjName;
        char* rootTriggerName;
        char* timestamp;
    };

    struct RawObject
    {
        int name;
        int type;
        int parent;
        int child;
        int nextSibling;
        int previousSibling;

        Vector3 scale;
        Vector3 rotation; 
        Vector3 position;

        Vector3 boundsMin;
        Vector3 boundsMax;

        int padding;
        int subdataPtr;
        int meshCount;

        // then for each mesh in mesh count (with a minimum of 1)
        // Material* material;
        // Mesh* mesh;
        // If meshCount == 0 then the first material* and mesh* are 0
    };

    struct VCDTable
    {
        int vertexCount;
        int lightColorCount;
        int colorCount;
        int uvCount;

        vec3<short>* vertices;
        Color* lightColors;
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
        int nameOffset; // Really a char* but its similar enough
        Color color;
        bool useVertexColor;
        bool unk_1;
        bool unk_2;
        bool useTexture;
        int textureInfoPtr; // From my old notes. Idfk what this is
        // The structure is 0x114 bytes long but idk what the rest of everything is atm so I'm just filling it with unknown floats (most of the values are floats)
        float unknowns[0x41];
    };

    struct MapTexture // Used exclusively in the map.dat file to describe a texture in textures.tpl
    {
        struct Info
        {
            u32 dataOffset;
            u32 unknown; // Padding?
            u8 unk_1;
            u8 unk_2;
            u8 unk_3;
            u8 unk_4;
        };

        u32 nameOffset;
        u8 unk_1;
        u8 unk_2;
        u8 unk_3;
        u8 unk_4;
        u16 width;
        u16 height;
        u32 unk_5;
    };

}
