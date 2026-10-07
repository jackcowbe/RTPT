#pragma once

#include <vulkan/vulkan.h>

class VulkanInstance
{
 public:
    VulkanInstance();
    ~VulkanInstance();

    VkInstance GetHandle() const;
    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;
 
 private:
    VkInstance m_Instance = VK_NULL_HANDLE;   

};