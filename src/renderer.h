#pragma once

#include "vulkan_context.h"

#include <vector>
#include <vulkan/vulkan.h>

class Renderer {
public:
  Renderer(VulkanContext *vulkan_context);
  ~Renderer();
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;
  void DrawFrame();

private:
  VkCommandPool m_CommandPool = VK_NULL_HANDLE;
  VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
  VkFence m_RenderFence = VK_NULL_HANDLE;
  VkSemaphore m_SwapchainSemaphore = VK_NULL_HANDLE;  
  VkSemaphore m_RenderSemaphore = VK_NULL_HANDLE;
  VulkanContext* m_vulkan_context = nullptr;
};