#include "vulkan_physicaldevice.h"
#include <stdexcept>


VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance instance)
{
    uint32_t PhysicalDeviceCount = 0;         //裝置數量
    VkResult result = vkEnumeratePhysicalDevices(instance, &PhysicalDeviceCount, nullptr); //先取得裝置數量
    if (result != VK_SUCCESS )
    {
         throw std::runtime_error("Failed to enumerate physical device count");
    }
    if (PhysicalDeviceCount == 0)
    {
        throw std::runtime_error("No physical devices found");
    }

    std::vector<VkPhysicalDevice> physicalDevicesVec(PhysicalDeviceCount);//使用vector來存放物理裝置

    VkResult result2 = vkEnumeratePhysicalDevices(instance, &PhysicalDeviceCount, physicalDevicesVec.data());
    if (result2 != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate physical devices");
    }

    SelectPhysicalDevice(physicalDevicesVec);
    
}

int VulkanPhysicalDevice::ScoreDevice(VkPhysicalDevice device) const
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device, &properties);

    switch(properties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return 100;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return 50;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return 20;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return 10;
        default:
            return 0;                 
    }
}

void VulkanPhysicalDevice::SelectPhysicalDevice( const std::vector<VkPhysicalDevice>& physicalDevices)
{
    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    int bestScore = -1;

    for (VkPhysicalDevice device : physicalDevices)
    {
        
        int score = ScoreDevice(device);
        if(bestDevice == VK_NULL_HANDLE || score > bestScore)
        {
            bestDevice = device;
            bestScore = score;
        }
    }
    if (bestDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable physical device");
    }
    m_PhysicalDevice = bestDevice;
}

VkPhysicalDevice VulkanPhysicalDevice::GetHandle() const
{
    return m_PhysicalDevice;
}
   
  