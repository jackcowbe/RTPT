#include "vulkan_context.h"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>

VulkanContext::VulkanContext(GLFWwindow* window)
{
    vkb::InstanceBuilder builder;

    auto instanceResult = builder
        .set_app_name("RTPT")
        .request_validation_layers()
        .use_default_debug_messenger()
        .build();

    if (!instanceResult)
    {
        throw std::runtime_error(
            "Failed to create Vulkan instance: " +
            instanceResult.error().message()
        );
    }

    vkb::Instance instance = instanceResult.value();
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkResult surfaceResult = glfwCreateWindowSurface(
        instance,
        window,
        nullptr,
        &surface
    );

    if (surfaceResult != VK_SUCCESS)
    {
        vkb::destroy_instance(instance);

        throw std::runtime_error(
            "Failed to create Vulkan surface: " +
            std::to_string(surfaceResult)
        );
    }

    vkb::PhysicalDeviceSelector selector{instance};

    auto physicalDeviceResult = selector
        .set_surface(surface)
        .select();

    if (!physicalDeviceResult)
    {
        vkb::destroy_surface(instance, surface);
        vkb::destroy_instance(instance);

        throw std::runtime_error(
            "Failed to select Vulkan physical device: " +
            physicalDeviceResult.error().message()
        );
    }

    vkb::PhysicalDevice physicalDevice = physicalDeviceResult.value();
    vkb::DeviceBuilder deviceBuilder{physicalDevice};

    auto deviceResult = deviceBuilder.build();

    if (!deviceResult)
    {
        vkb::destroy_surface(instance, surface);
        vkb::destroy_instance(instance);

        throw std::runtime_error(
            "Failed to create Vulkan logical device: " +
            deviceResult.error().message()
        );
    }

    vkb::Device device = deviceResult.value();

    auto graphicsQueueResult = device.get_queue(vkb::QueueType::graphics);
    auto presentQueueResult = device.get_queue(vkb::QueueType::present);
    auto graphicsQueueFamilyResult = device.get_queue_index(vkb::QueueType::graphics);
    auto presentQueueFamilyResult = device.get_queue_index(vkb::QueueType::present);

    if (!graphicsQueueResult ||
        !presentQueueResult ||
        !graphicsQueueFamilyResult ||
        !presentQueueFamilyResult)
    {
        vkb::destroy_device(device);
        vkb::destroy_surface(instance, surface);
        vkb::destroy_instance(instance);

        throw std::runtime_error(
            "Failed to get Vulkan graphics or present queue"
        );
    }

    vkb::SwapchainBuilder swapchainBuilder{device};
    auto swapchainResult = swapchainBuilder.build();

    if (!swapchainResult)
    {
        vkb::destroy_device(device);
        vkb::destroy_surface(instance, surface);
        vkb::destroy_instance(instance);

        throw std::runtime_error(
            "Failed to create Vulkan swapchain: " +
            swapchainResult.error().message()
        );
    }

    vkb::Swapchain swapchain = swapchainResult.value();
    auto imagesAndViewsResult = swapchain.get_images_and_image_views();

    if (!imagesAndViewsResult)
    {
        vkb::destroy_swapchain(swapchain);
        vkb::destroy_device(device);
        vkb::destroy_surface(instance, surface);
        vkb::destroy_instance(instance);

        throw std::runtime_error(
            "Failed to get Vulkan swapchain images and image views: " +
            imagesAndViewsResult.error().message()
        );
    }

    m_Instance = instance;
    m_Surface = surface;
    m_PhysicalDevice = physicalDevice;
    m_Device = device;
    m_GraphicsQueue = graphicsQueueResult.value();
    m_PresentQueue = presentQueueResult.value();
    m_GraphicsQueueFamily = graphicsQueueFamilyResult.value();
    m_PresentQueueFamily = presentQueueFamilyResult.value();
    m_Swapchain = swapchain;
    m_SwapchainImages = imagesAndViewsResult.value().first;
    m_SwapchainImageViews = imagesAndViewsResult.value().second;
}

VulkanContext::~VulkanContext()
{
    m_Swapchain.destroy_image_views(m_SwapchainImageViews);
    vkb::destroy_swapchain(m_Swapchain);
    vkb::destroy_device(m_Device);
    vkb::destroy_surface(m_Instance, m_Surface);
    vkb::destroy_instance(m_Instance);
}
