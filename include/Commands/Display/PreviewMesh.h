#pragma once

#include "Commands/Display/VertexAttribute.h"
#include "assimp/mesh.h"
    struct PreviewMesh {

            const char* m_Name;
            uint mTextureIndex;

            display_vertex_attribute_type_t attributes;
            uint m_VAO;
            uint m_VBO;
            uint m_EBO;
            uint m_IndexCount;
            uint m_VertexCount;
    };

PreviewMesh preview_mesh_from_ai_mesh(const aiMesh* mesh);
void preview_mesh_destroy();

void preview_mesh_draw(PreviewMesh* mesh);
void preview_mesh_flatten_vertex_array(void* output, const aiMesh* mesh,  display_vertex_attribute_type_t attributes, const int stride);

