#pragma once

#include <vulkan/vulkan.h>
#include "vulkan_physicaldevice.h"

class VulkanDevice
{
  public:
    VulkanDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices queueFamilyIndices);
    ~VulkanDevice();

    VkDevice GetHandle() const;
    VkQueue GetGraphicsQueue() const;
    VkQueue GetPresentQueue() const;

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete; 
    
  private:
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;  
};