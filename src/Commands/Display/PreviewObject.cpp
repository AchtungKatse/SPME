#include "Commands/Display/PreviewObject.h"
#include "Commands/Display/ShaderProgram.h"
#include "assimp/material.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace SPMEditor {

    PreviewObject::PreviewObject(const aiScene* scene, aiNode* node) : mAnimPosition(0), mAnimRotation(0), mAnimScale(1) {
        name = node->mName.C_Str();

        // Skip rendering colliders
        if (node->mName.length == 1 && node->mName.C_Str()[0] == 'A')
            return;

        // Get transform data (incompatable by default w/ glm)
        node->mTransformation.Decompose(*(aiVector3D*)&m_Scale, *(aiVector3D*)&m_Rotation, *(aiVector3D*)&m_Position);

        m_Meshes.reserve(node->mNumMeshes);
        m_Children.reserve(node->mNumChildren);

        // Load all meshes
        for (u32 i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            PreviewMesh* previewMesh = new PreviewMesh(mesh);

            aiString path;
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            if (mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), path) == AI_SUCCESS)
            {
                std::pair<const aiTexture*, int> textureIndexPair = scene->GetEmbeddedTextureAndIndex(path.C_Str());
                previewMesh->mTextureIndex = textureIndexPair.second;
            }

            m_Meshes.emplace_back(previewMesh);
        }

        // Then load children recursively
        for (u32 i = 0; i < node->mNumChildren; i++) {
            m_Children.emplace_back(PreviewObject(scene, node->mChildren[i]));
        }
    }

    void PreviewObject::Draw(ShaderProgram& program, glm::mat4 parentMatrix, PreviewTexture* textures) {
        // Create model matrix
        glm::vec3 pos   = m_Position + mAnimPosition;
        glm::vec3 rot   = m_Rotation + mAnimRotation;
        glm::vec3 scale = m_Scale    * mAnimScale;

        glm::mat4 translation = glm::translate(glm::mat4(1), pos);
        glm::mat4 rotation = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
        rotation = glm::rotate(rotation, rot.y, glm::vec3(0, 1, 0));
        rotation = glm::rotate(rotation, rot.z, glm::vec3(0, 0, 1));

        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1), scale);
        glm::mat4 matrix = parentMatrix * translation * rotation * scaleMatrix;

        // Set the matrix to render
        program.SetUniformMatrix4fv("model", matrix);
        for (size_t i = 0; i < m_Meshes.size(); i++) {
            if (m_Meshes[i]->mTextureIndex != 0xFFFFFFFF) {
                textures[m_Meshes[i]->mTextureIndex].Bind(0);
            }
            m_Meshes[i]->Draw();
        }

        for (size_t i = 0; i < m_Children.size(); i++) {
            m_Children[i].Draw(program, matrix, textures);
        }
    }

    PreviewObject* PreviewObject::FindNode(const char* name) {
        if (this->name)
            if (strcmp(this->name, name) == 0)
                return this;

        for (size_t i = 0; i < this->m_Children.size(); i++) {
            PreviewObject* node = m_Children[i].FindNode(name);
            if (node)
                return node;
        }

        return nullptr;
    }

    PreviewObject::~PreviewObject() {
    }
}
