    #include "vulkan_surface.h"
    #include <stdexcept>


VulkanSurface::VulkanSurface(VkInstance instance, GLFWwindow* window): m_Instance(instance)
{
   VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &m_Surface); 
    if (result != VK_SUCCESS)
     {
          throw std::runtime_error("Failed to create Vulkan surface");
     }
}

VkSurfaceKHR VulkanSurface::GetHandle() const
{
    return m_Surface;
}

VulkanSurface::~VulkanSurface()
{
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
}