#pragma once

typedef enum : unsigned int {
    DISPLAY_SHADER_TYPE_FRAGMENT = GL_FRAGMENT_SHADER,
    DISPLAY_SHADER_TYPE_VERTEX = GL_VERTEX_SHADER
} display_shader_type_t;

struct display_shader_t {
    u32 shader_index;
};

display_shader_t display_shader_create_from_source(const char* shaderName, const char* shaderSource, display_shader_type_t type);
void display_shader_destroy(display_shader_t shader);

