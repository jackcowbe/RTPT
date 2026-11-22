#include "window.h"
#include "vulkan_context.h"

int main() {
  
    WindowConfig config = {800, 600, "RTPT"};
    Window window(config);
    VulkanContext vulkanContext(window.GetHandle());


    while (!window.ShouldClose())
    {
        window.PollEvents();
    }


    return 0;
}
