#include "Commands/Display/VertexAttribute.h"


u32 display_vertex_attribute_get_size(display_vertex_attribute_type_t attribute) {

    switch (attribute) {
        default:
            return 0;
        case DISPLAY_VERTEX_ATTRIBUTE_POSITION:
            return sizeof(vec3);
        case DISPLAY_VERTEX_ATTRIBUTE_NORMAL:
            return sizeof(vec3);
        case DISPLAY_VERTEX_ATTRIBUTE_UV:
            return sizeof(vec2);
        case DISPLAY_VERTEX_ATTRIBUTE_UV2:
            return sizeof(vec2);
        case DISPLAY_VERTEX_ATTRIBUTE_UV3:
            return sizeof(vec2);
        case DISPLAY_VERTEX_ATTRIBUTE_UV4:
            return sizeof(vec2);
        case DISPLAY_VERTEX_ATTRIBUTE_COLOR:
            return sizeof(vec4);
        case DISPLAY_VERTEX_ATTRIBUTE_COLOR_2:
            return sizeof(vec4);
        case DISPLAY_VERTEX_ATTRIBUTE_COLOR_3:
            return sizeof(vec4);
        case DISPLAY_VERTEX_ATTRIBUTE_COLOR_4:
            return sizeof(vec4);
    }

    return 0;
}

u32 display_vertex_attribute_get_element_count(display_vertex_attribute_type_t attribute) {
    switch (attribute) {
        default:
            return 0;
        case DISPLAY_VERTEX_ATTRIBUTE_NONE:
            return 0;
        case DISPLAY_VERTEX_ATTRIBUTE_POSITION:
            return 3;
        case DISPLAY_VERTEX_ATTRIBUTE_NORMAL:
            break;
        case DISPLAY_VERTEX_ATTRIBUTE_UV:
            return 2;
        case DISPLAY_VERTEX_ATTRIBUTE_UV2:
            return 2;
        case DISPLAY_VERTEX_ATTRIBUTE_UV3:
            return 2;
        case DISPLAY_VERTEX_ATTRIBUTE_UV4:
            return 2;
        case DISPLAY_VERTEX_ATTRIBUTE_COLOR:
            return 4;
        case DISPLAY_VERTEX_ATTRIBUTE_COLOR_2:
            return 4;
        case DISPLAY_VERTEX_ATTRIBUTE_COLOR_3:
            return 4;
        case DISPLAY_VERTEX_ATTRIBUTE_COLOR_4:
            return 4;
    }

    return 0;
}
