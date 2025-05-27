#include "Window.h"
#include "Renderer.h"
#include <spdlog/spdlog.h>
#include <chrono>

int main() {
    const uint32_t WIDTH = 1920;
    const uint32_t HEIGHT = 1080;

#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);
#else
    spdlog::set_level(spdlog::level::debug);
#endif
    Window window(WIDTH, HEIGHT, "Vulkan Demo Ryan Mus");

    Renderer renderer(&window);
    renderer.initialize();

    auto lastTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;

    while (!window.shouldClose()) {
        window.pollEvents();
        renderer.drawFrame();

        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;

        if (elapsed.count() >= 1.0f) {
            float fps = frameCount / elapsed.count();
            spdlog::info("FPS: {:.2f}", fps);
            frameCount = 0;
            lastTime = currentTime;
        }
    }

    vkDeviceWaitIdle(renderer.getDevice());

    return 0;
}
