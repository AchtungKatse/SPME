#include "Commands/Display/VertexAttribute.h"

namespace SPMEditor 
{
    VertexAttribute::VertexAttribute(Type type, uint offset, uint size) : type(type), offset(offset), size(size) {

        if (size == 0)
        {
            switch (type) {
                case NONE:
                    this->size = 0;
                    break;
                case POSITION:
                    this->size = sizeof(Vector3);
                    break;
                case NORMAL:
                    this->size = sizeof(Vector3);
                    break;
                case UV:
                    this->size = sizeof(Vector2);
                    break;
                case UV2:
                    this->size = sizeof(Vector2);
                    break;
                case UV3:
                    this->size = sizeof(Vector2);
                    break;
                case UV4:
                    this->size = sizeof(Vector2);
                    break;
                case COLOR:
                    this->size = sizeof(aiColor4D);
                    break;
                case COLOR_2:
                    this->size = sizeof(aiColor4D);
                    break;
                case COLOR_3:
                    this->size = sizeof(aiColor4D);
                    break;
                case COLOR_4:
                    this->size = sizeof(aiColor4D);
                    break;
            }
        }
    }

    uint VertexAttribute::GetElementCount() const {
        switch (type) {
            case NONE:
                return 0;
            case POSITION:
                return 3;
            case NORMAL:
                break;
            case UV:
                return 2;
            case UV2:
                return 2;
            case UV3:
                return 2;
            case UV4:
                return 2;
            case COLOR:
                return 4;
            case COLOR_2:
                return 4;
            case COLOR_3:
                return 4;
            case COLOR_4:
                return 4;
        }
        return 0;
    }

    Vertex::Vertex() { }
    Vertex::Vertex(Vector3 position, Vector3 normal, Vector3 color, Vector2 uv) : 
        position(position), normal(normal), color(color), uv(uv)
    { }
}

