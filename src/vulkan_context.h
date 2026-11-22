#pragma once

#include <VkBootstrap.h>
#include <vector>

struct GLFWwindow;

class VulkanContext
{
public:
    explicit VulkanContext(GLFWwindow* window);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

private:
    vkb::Instance m_Instance{};
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    vkb::PhysicalDevice m_PhysicalDevice{};
    vkb::Device m_Device{};
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamily = 0;
    uint32_t m_PresentQueueFamily = 0;
    vkb::Swapchain m_Swapchain{};
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
};
