#include "Window.h"
#include "Renderer.h"

int main() {
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    Window window(WIDTH, HEIGHT, "Vulkan Application");

    Renderer renderer(&window);
    renderer.initialize();

    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.drawFrame();
    }

    vkDeviceWaitIdle(renderer.getDevice());

    return 0;
}
