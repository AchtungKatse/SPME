#pragma once

#include "defines.h"
SPME_HEADER_TOP

typedef enum {
    DISPLAY_VERTEX_ATTRIBUTE_NONE       = 0,
    DISPLAY_VERTEX_ATTRIBUTE_POSITION   = 1 << 0,
    DISPLAY_VERTEX_ATTRIBUTE_NORMAL     = 1 << 1,
    DISPLAY_VERTEX_ATTRIBUTE_UV         = 1 << 2,
    DISPLAY_VERTEX_ATTRIBUTE_UV2        = 1 << 3,
    DISPLAY_VERTEX_ATTRIBUTE_UV3        = 1 << 4,
    DISPLAY_VERTEX_ATTRIBUTE_UV4        = 1 << 5,
    DISPLAY_VERTEX_ATTRIBUTE_COLOR      = 1 << 6,
    DISPLAY_VERTEX_ATTRIBUTE_COLOR_2    = 1 << 7,
    DISPLAY_VERTEX_ATTRIBUTE_COLOR_3    = 1 << 8,
    DISPLAY_VERTEX_ATTRIBUTE_COLOR_4    = 1 << 9,
    DISPLAY_VERTEX_ATTRIBUTE_END        = 1 << 9
} display_vertex_attribute_type_t;

u32 display_vertex_attribute_get_size(display_vertex_attribute_type_t attribute);
u32 display_vertex_attribute_get_element_count(display_vertex_attribute_type_t attribute);

typedef struct {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} vertex_t;

SPME_HEADER_BOTTOM
