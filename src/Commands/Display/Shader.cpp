#include "Commands/Display/Shader.h"

namespace SPMEditor
{
    namespace 
    {
        const char* DefaultVertex = 
            "#version 330 core\n"
            "layout (location = 0) in vec3 position;\n"
            "layout (location = 1) in vec3 input_normal;\n"
            "layout (location = 2) in vec2 input_uv;\n"

            "out vec3 normal;\n"
            "out vec2 uv;\n"

            "uniform mat4 model;\n"
            "uniform mat4 g_ViewProjection;\n"

            "void main()\n"
            "{\n"
            "gl_Position = g_ViewProjection * model * vec4(position, 1.0);\n"
            "normal = input_normal;\n"
            "uv = input_uv;\n"
            "};";

        const char* DefaultFragment = 
            "#version 330\n"
            "in vec3 normal;\n"

            "out vec4 FragColor;\n"

            "uniform vec3 LightDir;\n"

            "void main() \n"
            "{ \n"
            "FragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f) * ((1 + dot(normal, LightDir)) / 2);\n"
            "}";

        inline const char* GetDefaultShader(const ShaderType type)
        {
            switch (type)
            {
                default:
                    LogError("Undefined default shader type: {}", (int)type);
                    return nullptr;
                case ShaderType::Fragment:
                    return DefaultFragment;
                case ShaderType::Vertex:
                    return DefaultVertex;
            }
        }
    }

    /*Shader::Shader(const char* shaderName, const char* filePath, ShaderType shaderType)*/
    /*{*/
    /*    if (filePath == nullptr)*/
    /*    {*/
    /*        Create(shaderName, nullptr, shaderType);*/
    /*        return;*/
    /*    }*/
    /**/
    /*    char error[256];*/
    /*    memset(error, 0, sizeof(error));*/
    /*    const char* source = stb_include_file((char*)filePath, "Help", "res/shaders", error);*/
    /*    CoreAssert(strlen(error) == 0, "Shader Include Error: {}", error);*/
    /**/
    /**/
    /*    Create(shaderName, source, shaderType);*/
    /*    free((void*)source);*/
    /*}*/

    Shader Shader::CreateFromSource(const char* shaderName, const char* shaderSource, ShaderType type)
    {
        Shader shader;
        shader.Create(shaderName, shaderSource, type);
        return shader;
    }

    void Shader::Create(const char* name, const char* source, ShaderType shaderType)
    {
        if (source == nullptr)
            source = GetDefaultShader(shaderType);


        Assert(source != nullptr, "Shader source is null and failed to resolve default. Source: {}", source);

        m_ShaderIndex = glCreateShader(static_cast<unsigned int>(shaderType));

        glShaderSource(m_ShaderIndex, 1, &source, NULL);
        LogTrace("Created shader {} (Type: {}) with index {}", name, (uint)shaderType, m_ShaderIndex);

        glCompileShader(m_ShaderIndex);

        // Log Error
        int success;
        int logSize;
        glGetShaderiv(m_ShaderIndex, GL_INFO_LOG_LENGTH, &logSize);

        char infoLog[logSize];
        glGetShaderiv(m_ShaderIndex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(m_ShaderIndex, logSize, NULL, infoLog);
            LogError("Failed to compile shader '{0}': {1}", name, (char*)infoLog, source);

            int sourceOffset = 0;
            int lineOffset = 0;
            int lineNumber = 1;
            char lineBuffer[512];
            char c = source[sourceOffset++];
            while (c != 0)
            {
                lineBuffer[lineOffset++] = c;
                c = source[sourceOffset++];

                while (c == '\n')
                {
                    lineBuffer[lineOffset++] = 0;
                    LogError("\tLine {}: {}", lineNumber++, (char*)lineBuffer);
                    c = source[sourceOffset++];
                    lineOffset = 0;
                }
            }

            // Use the default shader instead
            switch (shaderType)
            {
                default: return;
                case ShaderType::Fragment:
                         source = DefaultFragment;
                         break;
                case ShaderType::Vertex:
                         source = DefaultVertex;
                         break;
            }

            glDeleteShader(m_ShaderIndex);

            m_ShaderIndex = glCreateShader(static_cast<unsigned int>(shaderType));
            glShaderSource(m_ShaderIndex, 1, &source, NULL);
            glCompileShader(m_ShaderIndex);

            // If it fails again I'm going to stab someone
            glGetShaderiv(m_ShaderIndex, GL_INFO_LOG_LENGTH, &logSize);
            glGetShaderiv(m_ShaderIndex, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                LogError("Failed to compile default shader");
                LogError("Failed to compile shader '{0}': {1}\n'{2}'", name, (char*)infoLog, source);
            }
        }
        else
        {
            LogTrace("\tSuccessfully compiled shader '{0}' (Index: {1})", name, m_ShaderIndex);
        }
    }

    Shader::~Shader()
    {
        glDeleteShader(m_ShaderIndex);
    }
    /**/
    /*FragmentShader::FragmentShader(const char* shaderName, const char* filePath) : Shader(shaderName, filePath, ShaderType::Fragment) { }*/
    /*VertexShader::VertexShader(const char* shaderName, const char* filePath) : Shader(shaderName, filePath, ShaderType::Vertex) { }*/
}

