#include "window.h"
#include <stdexcept>

Window::Window(const WindowConfig& config)
    : m_Width(config.width),
      m_Height(config.height)
{
    int initResult = glfwInit();

    if (initResult != GLFW_TRUE)
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = glfwCreateWindow(
        m_Width,
        m_Height,
        config.title,
        nullptr,
        nullptr
    );

    if (!m_Window)
    {
        throw std::runtime_error(
            "Failed to create GLFW window"
        );
    }
}

Window::~Window()
{
    if (m_Window != nullptr)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    glfwTerminate();
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_Window) == GLFW_TRUE;
}

void Window::PollEvents()
{
    glfwPollEvents();
}

GLFWwindow* Window::GetHandle() const
{
    return m_Window;
}
