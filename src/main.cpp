#include "window.h"
#include "vulkan_instance.h"
#include "vulkan_surface.h"
#include "vulkan_physicaldevice.h"
#include <iostream>

int main() {
  
    WindowConfig config = {800, 600, "RTPT"};
    Window window(config);
    VulkanInstance instance;
    VulkanSurface surface(instance.GetHandle(), window.GetHandle());
    VulkanPhysicalDevice physicalDevice(instance.GetHandle(), surface.GetHandle());


    while (!window.ShouldClose())
    {
        window.PollEvents();
    }


    return 0;
}
