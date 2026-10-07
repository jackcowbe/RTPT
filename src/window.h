#pragma once

#include <GLFW/glfw3.h>

struct WindowConfig {
    int width;
    int height;
    const char* title;
};

class Window
{
public:
    Window(const WindowConfig& config);
    ~Window();

    bool ShouldClose() const;
    void PollEvents();

    GLFWwindow* GetHandle() const;

private:
    GLFWwindow* m_Window = nullptr;

    int m_Width;
    int m_Height;
};

