#include "Commands/Display/Shader.h"

// ===============================================
// Private Functions
// ===============================================

    void display_shader_create_internal(const char* name, const char* source, display_shader_type_t type);

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

        inline const char* GetDefaultShader(const display_shader_type_t type)
        {
            switch (type)
            {
                default:
                    LogError("Undefined default shader type: %d", (int)type);
                    return nullptr;
                case display_shader_type_t::DISPLAY_SHADER_TYPE_FRAGMENT:
                    return DefaultFragment;
                case display_shader_type_t::DISPLAY_SHADER_TYPE_VERTEX:
                    return DefaultVertex;
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
    /*    CoreAssert(strlen(error) == 0, "Shader Include Error: %d", error);*/
    /**/
    /**/
    /*    Create(shaderName, source, shaderType);*/
    /*    free((void*)source);*/
    /*}*/

    display_shader_t display_shader_t::display_shader_create_from_source(const char* shaderName, const char* shaderSource, display_shader_type_t type)
    {
        display_shader_t shader;
        shader.display_shader_create(shaderName, shaderSource, type);
        return shader;
    }

    void display_shader_t::display_shader_create(const char* name, const char* source, display_shader_type_t shaderType)
    {
        if (source == nullptr)
            source = GetDefaultShader(shaderType);


        Assert(source != nullptr, "Shader source is null and failed to resolve default. Source: %s", source);

        shader_index = glCreateShader(static_cast<unsigned int>(shaderType));

        glShaderSource(shader_index, 1, &source, NULL);
        LogTrace("Created shader %s (Type: %u) with index %u", name, (uint)shaderType, shader_index);

        glCompileShader(shader_index);

        // Log Error
        int success;
        int logSize;
        glGetShaderiv(shader_index, GL_INFO_LOG_LENGTH, &logSize);

        char infoLog[logSize];
        glGetShaderiv(shader_index, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader_index, logSize, NULL, infoLog);
            LogError("Failed to compile shader '%s': %s", name, (char*)infoLog, source);

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
                    LogError("\tLine %d: %s", lineNumber++, (char*)lineBuffer);
                    c = source[sourceOffset++];
                    lineOffset = 0;
                }
            }

            // Use the default shader instead
            switch (shaderType)
            {
                default: return;
                case display_shader_type_t::DISPLAY_SHADER_TYPE_FRAGMENT:
                         source = DefaultFragment;
                         break;
                case display_shader_type_t::DISPLAY_SHADER_TYPE_VERTEX:
                         source = DefaultVertex;
                         break;
            }

            glDeleteShader(shader_index);

            shader_index = glCreateShader(static_cast<unsigned int>(shaderType));
            glShaderSource(shader_index, 1, &source, NULL);
            glCompileShader(shader_index);

            // If it fails again I'm going to stab someone
            glGetShaderiv(shader_index, GL_INFO_LOG_LENGTH, &logSize);
            glGetShaderiv(shader_index, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                LogError("Failed to compile default shader");
                LogError("Failed to compile shader '{0}': {1}\n'{2}'", name, (char*)infoLog, source);
            }
        }
        else
        {
            LogTrace("\tSuccessfully compiled shader '{0}' (Index: {1})", name, shader_index);
        }
    }

void display_shader_destroy(display_shader_t shader) {
    glDeleteShader(shader.shader_index);
}
