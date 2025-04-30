#pragma once

namespace SPMEditor 
{
    struct VertexAttribute {
        enum Type {
            NONE       = 0,
            POSITION   = 1 << 0,
            NORMAL     = 1 << 1,
            UV         = 1 << 2,
            UV2        = 1 << 3,
            UV3        = 1 << 4,
            UV4        = 1 << 5,
            COLOR      = 1 << 6,
            COLOR_2    = 1 << 7,
            COLOR_3    = 1 << 8,
            COLOR_4    = 1 << 9,
            END        = 1 << 9
        };

        VertexAttribute() = default;
        VertexAttribute(Type type, uint offset, uint size = 0);
        uint GetElementCount() const;

        Type type;
        uint offset;
        uint size;

    };

    struct Vertex
    {
        Vertex();
        Vertex(Vector3 position, Vector3 normal = Vector3(), Vector3 color = Vector3(), Vector2 uv = Vector2());

        Vector3 position;
        Vector3 normal;
        Vector3 color;
        Vector2 uv;
    };
}

