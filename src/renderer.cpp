#include "renderer.h"
#include <stdexcept>
#include <string>
#include <cstdint>

Renderer::Renderer(VulkanContext *vulkan_context)
    : m_vulkan_context(vulkan_context) {
  if (!m_vulkan_context) {
    throw std::invalid_argument("VulkanContext pointer is null");
  }

  VkDevice device = m_vulkan_context->GetDevice();

  auto cleanup = [&]() {
    if (m_RenderSemaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device, m_RenderSemaphore, nullptr);
      m_RenderSemaphore = VK_NULL_HANDLE;
    }
    if (m_SwapchainSemaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device, m_SwapchainSemaphore, nullptr);
      m_SwapchainSemaphore = VK_NULL_HANDLE;
    }
    if (m_RenderFence != VK_NULL_HANDLE) {
      vkDestroyFence(device, m_RenderFence, nullptr);
      m_RenderFence = VK_NULL_HANDLE;
    }
    if (m_CommandPool != VK_NULL_HANDLE) {
      vkDestroyCommandPool(device, m_CommandPool, nullptr);
      m_CommandPool = VK_NULL_HANDLE;
    }
  };

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = m_vulkan_context->GetGraphicsQueueFamily();

  VkResult result =
      vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool: " +
                             std::to_string(result));
  }

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  result = vkAllocateCommandBuffers(device, &allocInfo, &m_CommandBuffer);
  if (result != VK_SUCCESS) {
    cleanup();
    throw std::runtime_error("Failed to allocate command buffer: " +
                             std::to_string(result));
  }

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  result = vkCreateFence(device, &fenceInfo, nullptr, &m_RenderFence);
  if (result != VK_SUCCESS) {
    cleanup();
    throw std::runtime_error("Failed to create render fence: " +
                             std::to_string(result));
  }

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  result =
      vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_SwapchainSemaphore);
  if (result != VK_SUCCESS) {
    cleanup();
    throw std::runtime_error("Failed to create swapchain semaphore: " +
                             std::to_string(result));
  }

  result =
      vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderSemaphore);
  if (result != VK_SUCCESS) {
    cleanup();
    throw std::runtime_error("Failed to create render semaphore: " +
                             std::to_string(result));
  }
}

void Renderer::DrawFrame(){
  VkDevice device = m_vulkan_context->GetDevice();
  VkSwapchainKHR swapchain = m_vulkan_context->GetSwapchain();

  vkWaitForFences(device, 1, &m_RenderFence, VK_TRUE, UINT64_MAX);

  uint32_t imageIndex = 0;
  VkResult acquireResult = vkAcquireNextImageKHR(
    device, swapchain, UINT64_MAX, m_SwapchainSemaphore, VK_NULL_HANDLE, &imageIndex);
  
  if(acquireResult == VK_ERROR_OUT_OF_DATE_KHR){
    return;
  }
  if(acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR){
    throw std::runtime_error("Failed to acquire swapchain image");
  }
  
  vkResetFences(device, 1, &m_RenderFence);
  vkResetCommandBuffer(m_CommandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VkImage image = m_vulkan_context->GetSwapchainImages()[imageIndex];

  VkImageSubresourceRange range{};
  range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  range.baseMipLevel = 0;
  range.levelCount = 1;
  range.baseArrayLayer = 0;
  range.layerCount = 1; 
  
}



Renderer::~Renderer() {
  VkDevice device = m_vulkan_context->GetDevice();
  vkDeviceWaitIdle(device);

  if (m_RenderSemaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(device, m_RenderSemaphore, nullptr);
  }
  if (m_SwapchainSemaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(device, m_SwapchainSemaphore, nullptr);
  }
  if (m_RenderFence != VK_NULL_HANDLE) {
    vkDestroyFence(device, m_RenderFence, nullptr);
  }
  if (m_CommandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, m_CommandPool, nullptr);
  }
}
