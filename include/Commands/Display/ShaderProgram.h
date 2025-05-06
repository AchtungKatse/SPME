#ifndef SPME_NO_VIEWER

#pragma once
#include "Commands/Display/Shader.h"
#include "glad/glad.h"
#include "glm/mat3x3.hpp"
#include "glm/mat4x4.hpp"
#include <string>

namespace SPMEditor
{
    class ShaderProgram
    {
        public:
            ShaderProgram();
            ShaderProgram(const char* programName, std::initializer_list<Shader> shaders);
            ~ShaderProgram();


            void UseProgram();

            uint GetUniform(const char* name);
            void SetUniformMatrix4fv(const char* name, const glm::mat4& matrix);
            void SetUniformMatrix3fv(const char* name, const glm::mat3& matrix);
            void SetUniformInt(const char* name, int value);
            void SetUniformUInt(const char* name, uint value);
            void SetUniformFloat(const char* name, float value);
            void SetUniformVector2(const char* name, Vector2 value);
            void SetUniformVector3i(const char* name, Vector3i value);
            void SetUniformVector3(const char* name, Vector3 value);
            /*void SetUniformVector4(const char* name, Vector4 value);*/

            inline uint GetProgramId() { return m_ShaderProgramId; }

        private:
            std::unordered_map<std::string, GLuint> m_UniformLocations;
            const char* m_ShaderName;
            unsigned int m_ShaderProgramId;

            bool m_UseRenderContext;
    };
}


#endif

