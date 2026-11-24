#pragma once

#include <VkBootstrap.h>
#include <vector>

struct GLFWwindow;

class VulkanContext {
public:
  explicit VulkanContext(GLFWwindow *window);
  ~VulkanContext();

  VulkanContext(const VulkanContext &) = delete;
  VulkanContext &operator=(const VulkanContext &) = delete;

  VkDevice GetDevice() const { return m_Device.device; }
  VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
  uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
  VkSwapchainKHR GetSwapchain() const { return m_Swapchain.swapchain; }
  const std::vector<VkImage> &GetSwapchainImages() const {
    return m_SwapchainImages;
  }
  VkQueue GetPresentQueue() const {return m_PresentQueue;}

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
