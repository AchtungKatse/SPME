#pragma once
#include "Commands/Display/PreviewObject.h"
#include "FileTypes/LevelData.h"
#include "GLFW/glfw3.h"

namespace SPMEditor {
    class Display {
        public:
            Display() = delete;

            static void DisplayLevel(LevelData& level);

        private:
            static void InitOpenGL();
            static void MouseMoveCallback(GLFWwindow* window, double xPos, double yPos);
            static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
            static void MouseButtonCallback(GLFWwindow* window, int buttonIndex, int action, int mods);
            static void ResizeWindowCallback(GLFWwindow* window, int width, int height);
            static void InitializeCallbacks(GLFWwindow* window);

            static constexpr uint GLMajorVersion = 3;
            static constexpr uint GLMinorVersion = 3;
            static double s_DeltaTime;
            static int s_ScreenWidth, s_ScreenHeight;
            static bool s_Initialized;
            static const char* s_VertexSource;
            static const char* s_FragmentSource;
    };
}
