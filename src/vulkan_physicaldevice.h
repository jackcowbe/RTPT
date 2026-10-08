#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanPhysicalDevice
{
 public:
    VulkanPhysicalDevice(VkInstance instance);
    ~VulkanPhysicalDevice() = default;

    VkPhysicalDevice GetHandle() const;
    

    VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
    VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;

 private:
    void SelectPhysicalDevice(const std::vector<VkPhysicalDevice>& physicalDevices );
    int ScoreDevice(VkPhysicalDevice device) const;
    bool HasGraphicsQueueFamily(VkPhysicalDevice device) const;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;  
};