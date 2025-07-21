# Vulkan PBR deferred Renderer #

Name : Ryan Mus

Git : https://github.com/Howest-DAE-GD/graphics-programming-2-vulkan-project-Ryan-Mus

## Description ##

A physically-based rendering engine built with Vulkan that demonstrates modern real-time graphics techniques. This application renders 3D models with realistic lighting and materials using a deferred rendering pipeline.
## Features ##

•	**Modern Vulkan Implementation:** Utilizes Vulkan 1.3 features including dynamic rendering and synchronization2

•	**PBR Materials**: Full physically-based rendering pipeline supporting metallic-roughness workflow

•	**HDR Rendering**: High dynamic range rendering with ACES tone mapping

•	**Image-Based Lighting**: Environment map-based ambient lighting using IBL cubemaps

•	**Dynamic Lighting**: Real-time directional and point light support with shadow mapping

**Camera Controls:**

•	WASD for movement, Space/Shift for up/down

•	Right-click + mouse for camera rotation

•	Photographic exposure controls (aperture, ISO, shutter speed)

•	**Visual Debugging:** Multiple rendering modes to visualize individual G-buffer components

**Optimizations:**

•	Deferred shading for efficient handling of multiple light sources

•	Compute shader-based post-processing

## Technical Details ##

•	**Architecture:** Renderer built with a modular design using builder patterns

•	**Memory Management:** Efficient GPU memory allocation using VMA (Vulkan Memory Allocator)

•	**Asset Loading:** Support for GLTF models with textures using Assimp

•	**Multi-Pass Rendering:** G-buffer generation, shadow mapping, final lighting pass, and tone mapping

•	**HDRI Support:** Environment map loading with real-time cubemap conversion

## Controls ##

•	**Movement:** WASD + Space/Shift

•	**Camera:** Right-click + drag to rotate

•	**Debug Views:** F1 (reset), F2 (cycle through G-buffer visualizations), F10 (cycle debug modes)

•	**Lighting Controls:**

•	I/K: Increase/decrease IBL intensity

•	O/L: Increase/decrease sun intensity

•	U/J: Adjust camera exposure (ISO)

The renderer showcases modern real-time rendering techniques with physically-accurate lighting calculations and material representation.
