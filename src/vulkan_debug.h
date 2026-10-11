#pragma once

#include <vulkan/vulkan.h>

class VulkanDebugMessenger
{
public:
    explicit VulkanDebugMessenger(VkInstance instance);
    ~VulkanDebugMessenger();

    VulkanDebugMessenger(const VulkanDebugMessenger&) = delete;
    VulkanDebugMessenger& operator=(const VulkanDebugMessenger&) = delete;

    static bool IsEnabled();
    static bool IsValidationLayerSupported();
    static const char* GetValidationLayerName();
    static VkDebugUtilsMessengerCreateInfoEXT MakeCreateInfo();

private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
};
