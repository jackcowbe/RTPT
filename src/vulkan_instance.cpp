#include "vulkan_instance.h"
#include "vulkan_debug.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>

VulkanInstance::VulkanInstance()
{
    const bool enableValidationLayers =
        VulkanDebugMessenger::IsEnabled();

    if (enableValidationLayers &&
        !VulkanDebugMessenger::IsValidationLayerSupported())
    {
        throw std::runtime_error(
            "Validation layer not supported");
    }
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "RTPT";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "my honey engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo =
        VulkanDebugMessenger::MakeCreateInfo();

    const char* validationLayer =
        VulkanDebugMessenger::GetValidationLayerName();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = enableValidationLayers ? &debugCreateInfo : nullptr;  
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = enableValidationLayers ? 1 : 0;
    createInfo.ppEnabledLayerNames =
        enableValidationLayers ? &validationLayer : nullptr;
    uint32_t extensionCount = 0;

    const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        
    if (requiredExtensions == nullptr)
    {
        throw std::runtime_error(
            "Failed to get GLFW required Vulkan extensions");
    }

    std::vector<const char*> extensions(
        requiredExtensions,
        requiredExtensions + extensionCount);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result =
        vkCreateInstance(&createInfo, nullptr, &m_Instance);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

VkInstance VulkanInstance::GetHandle() const
{
    return m_Instance;
}

VulkanInstance::~VulkanInstance()
{
    if (m_Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_Instance, nullptr);
        m_Instance = VK_NULL_HANDLE;
    }
}
