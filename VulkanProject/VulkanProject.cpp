#include "Window.h"
#include "Renderer.h"
#include <spdlog/spdlog.h>

int main() {
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);
#else
	spdlog::set_level(spdlog::level::debug);
#endif
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
