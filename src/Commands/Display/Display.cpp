#include "Commands/Display/Display.h"
#include "Commands/Display/PreviewObject.h"
#include "Commands/Display/PreviewTexture.h"
#include "Commands/Display/ShaderProgram.h"
#include "GLFW/glfw3.h"
#include "Types/Types.h"
#include "assimp/Importer.hpp"
#include "glad/glad.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include "stb_image.h"

namespace SPMEditor {
    GLFWwindow* s_Window;

    int Display::s_ScreenWidth = 720;
    int Display::s_ScreenHeight = 480;
    bool Display::s_Initialized;
    double Display::s_DeltaTime = 0.0;

    glm::vec3 s_CameraPosition;
    glm::vec3 s_EulerAngles;
    glm::vec3 s_InputDirection;
    float s_CameraSpeed = 10;

    void Display::DisplayLevel(LevelData& level) {
        if (!s_Initialized)
            InitOpenGL();

        // Load scene
        PreviewObject rootObject = PreviewObject(level.geometry, level.geometry->mRootNode);

        // Setup shader
        ShaderProgram defaultShader("Default shader", {
                Shader::CreateFromSource("Vertex", s_VertexSource, ShaderType::Vertex),
                Shader::CreateFromSource("Fragment", s_FragmentSource, ShaderType::Fragment)});

        glm::vec3 lightDir = glm::normalize(glm::vec3(.707, -.707, .5));
        defaultShader.UseProgram();
        defaultShader.SetUniformVector3("LightDir", *(Vector3*)&lightDir);

        // Load textures
        PreviewTexture* textures = new PreviewTexture[level.geometry->mNumTextures];
        stbi_set_flip_vertically_on_load(true);  

        for (int i = 0; i < level.geometry->mNumTextures; i++) {
            textures[i] = PreviewTexture();

            const aiTexture* tex = level.geometry->mTextures[i];
            int size, width, height, channels = 0;
            if (tex->mHeight == 0)
            {
                size = tex->mWidth;
                u8* data = stbi_load_from_memory((u8*)tex->pcData, size, &width, &height, &channels, 4);
                textures[i].Create(data, width, height, PreviewTexture::PixelFormat::RGBA8, PreviewTexture::WrapType::Repeat, PreviewTexture::FilterType::Nearest);
                stbi_image_free(data);
            }
            else {
                textures[i].Create(tex->pcData, tex->mWidth, tex->mHeight, PreviewTexture::PixelFormat::RGB8, PreviewTexture::WrapType::Repeat, PreviewTexture::FilterType::Nearest); 
            }
        }

        // Final gl init
        double lastTime = 0;
        int frame = 0;
        while (!glfwWindowShouldClose(s_Window))
        {
            // Frame init
            s_DeltaTime = glfwGetTime() - lastTime;
            lastTime = glfwGetTime();
            glfwPollEvents();

            // Camera 
            glm::mat4 translation = glm::translate(glm::mat4(1), s_CameraPosition);
            glm::mat4 rotation = glm::rotate(glm::mat4(1), s_EulerAngles.y, glm::vec3(0.0f, 1.0f, 0.0f));
            rotation = glm::rotate(rotation, s_EulerAngles.x, glm::vec3(1.0f, 0.0f, 0.0f));

            s_CameraPosition += rotation * glm::vec4(-s_InputDirection, 0) * (float)s_DeltaTime * s_CameraSpeed;

            glm::mat4 view = glm::inverse(translation * rotation);
            glm::mat4 project = glm::perspective(90.0f, (float)s_ScreenWidth / s_ScreenHeight, .1f, 1000.0f);

            // Shader setup
            defaultShader.UseProgram();
            defaultShader.SetUniformMatrix4fv("g_ViewProjection", project * view);

            // Rendering
            rootObject.Draw(defaultShader, glm::mat4(1), textures);

            // Frame cleanup
            glfwSwapBuffers(s_Window);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        glfwDestroyWindow(s_Window);
        glfwTerminate();

    }

    void Display::InitOpenGL() {
        int glfwInitialized = glfwInit();
        Assert(glfwInitialized, "Initalizing glfw");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GLMajorVersion);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GLMinorVersion);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Load monitor settings
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        s_ScreenHeight = mode->height;
        s_ScreenWidth = mode->width;

        // Create window
        s_Window = glfwCreateWindow(s_ScreenWidth, s_ScreenHeight, "Viewer", monitor, NULL);
        LogTrace("Created main window");
        Assert(s_Window, "Failed to create window");

        glfwMakeContextCurrent(s_Window);

        // Initialize GLAD
        int gladLoaded = gladLoadGL();
        Assert(gladLoaded == 1, "Failed to load glad GL: {0}");
        LogTrace("Initialized OpenGL");

        // Setup render environment
        glClearColor(.5f, .5f, .5f, 1.0f);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glfwSwapInterval(1);

        // Setup input
        glfwSetInputMode(s_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        InitializeCallbacks(s_Window);
    }

    void Display::MouseMoveCallback(GLFWwindow* window, double xPos, double yPos)
    {
        static Vector2 lastPosition;
        Vector2 delta = Vector2(lastPosition.x - xPos, lastPosition.y - yPos);
        lastPosition = Vector2(xPos, yPos);

        s_EulerAngles.y += delta.x * s_DeltaTime * (1.0 / 5);
        s_EulerAngles.x += delta.y * s_DeltaTime * (1.0 / 5);
    }

    void Display::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_SPACE) s_InputDirection.y = -1;
            if (key == GLFW_KEY_LEFT_SHIFT) s_InputDirection.y = 1;
            if (key == GLFW_KEY_W) s_InputDirection.z = 1;
            if (key == GLFW_KEY_S) s_InputDirection.z = -1;
            if (key == GLFW_KEY_A) s_InputDirection.x = 1;
            if (key == GLFW_KEY_D) s_InputDirection.x = -1;
            if (key == GLFW_KEY_E) s_CameraSpeed += 5;
            if (key == GLFW_KEY_Q) s_CameraSpeed -= 2;
        }
        else if (action == GLFW_RELEASE) {
            if (key == GLFW_KEY_SPACE) s_InputDirection.y = 0;
            if (key == GLFW_KEY_LEFT_SHIFT) s_InputDirection.y = 0;
            if (key == GLFW_KEY_W) s_InputDirection.z = 0;
            if (key == GLFW_KEY_S) s_InputDirection.z = 0;
            if (key == GLFW_KEY_A) s_InputDirection.x = 0;
            if (key == GLFW_KEY_D) s_InputDirection.x = 0;
            if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
        }
    }


    void Display::MouseButtonCallback(GLFWwindow* window, int buttonIndex, int action, int mods) {
    }

    void Display::ResizeWindowCallback(GLFWwindow* window, int width, int height)
    {
        s_ScreenWidth = width;
        s_ScreenHeight = height;

        glViewport(0, 0, width, height);
    }

    void Display::InitializeCallbacks(GLFWwindow* window)
    {
        LogInfo("Setting up glfw callbacks");
        glfwSetKeyCallback(window, Display::KeyCallback);
        glfwSetMouseButtonCallback(window, Display::MouseButtonCallback);
        glfwSetCursorPosCallback(window, Display::MouseMoveCallback);
        glfwSetWindowSizeCallback(window, Display::ResizeWindowCallback);
        glfwSetFramebufferSizeCallback(window, Display::ResizeWindowCallback);
    }

    const char* Display::s_VertexSource = 
        "#version 330 core\n"
        "layout (location = 0) in vec3 position;\n"
        "layout (location = 1) in vec3 inNormal;\n"
        "layout (location = 2) in vec4 inColor;\n"
        "layout (location = 3) in vec2 inUv;\n"

        "out vec3 normal;\n"
        "out vec4 color;\n"
        "out vec2 uv;\n"

        "uniform mat4 model;\n"
        "uniform mat4 g_ViewProjection;\n"

        "void main()\n"
        "{\n"
        "gl_Position = g_ViewProjection * model * vec4(position, 1.0);\n"
        "normal = inNormal;\n"
        "uv = inUv;\n"
        "color = inColor;\n"
        "};";

    const char* Display::s_FragmentSource = 
        "#version 330\n"
        "in vec3 normal;\n"
        "in vec4 color;\n"
        "in vec2 uv;\n"
        "out vec4 FragColor;\n"
        "uniform vec3 LightDir;\n"
        "uniform sampler2D _texture;"

        "void main() \n"
        "{ \n"
        "vec4 texColor = texture(_texture, uv);\n"
        "if (color.w * texColor.w <= .0) discard;\n"
        "FragColor = color * texColor;\n"
        "}";

}