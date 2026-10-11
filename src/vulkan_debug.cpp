#include "vulkan_debug.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
    constexpr const char* kValidationLayer =
        "VK_LAYER_KHRONOS_validation";

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void* pUserData)
    {
        std::cerr
            << "Validation layer: "
            << pCallbackData->pMessage
            << std::endl;

        return VK_FALSE;
    }
}

VulkanDebugMessenger::VulkanDebugMessenger(VkInstance instance)
    : m_Instance(instance)
{
    if (!IsEnabled())
    {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo = MakeCreateInfo();

    PFN_vkCreateDebugUtilsMessengerEXT createDebugMessenger =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(
                m_Instance,
                "vkCreateDebugUtilsMessengerEXT"));

    if (createDebugMessenger == nullptr)
    {
        throw std::runtime_error(
            "Failed to load vkCreateDebugUtilsMessengerEXT");
    }

    VkResult result = createDebugMessenger(
        m_Instance,
        &createInfo,
        nullptr,
        &m_Messenger);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to create debug messenger");
    }
}

VulkanDebugMessenger::~VulkanDebugMessenger()
{
    if (m_Messenger == VK_NULL_HANDLE)
    {
        return;
    }

    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(
                m_Instance,
                "vkDestroyDebugUtilsMessengerEXT"));

    if (destroyDebugMessenger != nullptr)
    {
        destroyDebugMessenger(
            m_Instance,
            m_Messenger,
            nullptr);
    }
}

bool VulkanDebugMessenger::IsEnabled()
{
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

bool VulkanDebugMessenger::IsValidationLayerSupported()
{
    uint32_t layerCount = 0;

    VkResult result =
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to enumerate instance layer count");
    }

    std::vector<VkLayerProperties> availableLayers(layerCount);

    result = vkEnumerateInstanceLayerProperties(
        &layerCount,
        availableLayers.data());

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(
            "Failed to enumerate instance layer properties");
    }

    for (const VkLayerProperties& layer : availableLayers)
    {
        if (std::strcmp(layer.layerName, kValidationLayer) == 0)
        {
            return true;
        }
    }

    return false;
}

const char* VulkanDebugMessenger::GetValidationLayerName()
{
    return kValidationLayer;
}

VkDebugUtilsMessengerCreateInfoEXT
VulkanDebugMessenger::MakeCreateInfo()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;

    return createInfo;
}
