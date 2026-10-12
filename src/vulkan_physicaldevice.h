#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <optional>


struct QueueFamilyIndices
{
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool IsComplete() const
   {
       return graphicsFamily.has_value() && presentFamily.has_value();
   }
};
class VulkanPhysicalDevice
{
 public:
    VulkanPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    ~VulkanPhysicalDevice() = default;

    VkPhysicalDevice GetHandle() const;

    QueueFamilyIndices GetQueueFamilyIndices() const;    

    VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
    VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;

 private:
    void SelectPhysicalDevice(const std::vector<VkPhysicalDevice>& physicalDevices , VkSurfaceKHR surface);
    int ScoreDevice(VkPhysicalDevice device) const;
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE; 
    QueueFamilyIndices m_QueueFamilyIndices; 
};

