#include "window.h"
#include "vulkan_context.h"
#include "renderer.h"

int main() {
  
    WindowConfig config = {800, 600, "RTPT"};
    Window window(config);
    VulkanContext vulkanContext(window.GetHandle());
    Renderer renderer(&vulkanContext);


    while (!window.ShouldClose())
    {
        window.PollEvents();
    }


    return 0;
}
