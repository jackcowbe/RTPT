#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

class VulkanSurface
{
 public:
    VulkanSurface(VkInstance instance, GLFWwindow* window);
    ~VulkanSurface();

    VkSurfaceKHR GetHandle() const;
    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;

 private:
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkInstance m_Instance = VK_NULL_HANDLE;
};
