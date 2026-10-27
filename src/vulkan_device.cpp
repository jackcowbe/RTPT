#include "vulkan_device.h"
#include <stdexcept>
#include <set>
#include <vector>

VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices queueFamilyIndices)
{
   std::set<uint32_t> uniqueQueueFamilies = {
    queueFamilyIndices.graphicsFamily.value(),
    queueFamilyIndices.presentFamily.value()
    };

 
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
    }

}