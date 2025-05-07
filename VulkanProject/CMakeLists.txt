cmake_minimum_required(VERSION 3.16)

project(VulkanProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND OR NOT Vulkan_INCLUDE_DIRS OR NOT Vulkan_LIBRARIES)
    message(FATAL_ERROR "Vulkan SDK not found!")
endif()

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4 # Nieuwste stabiele versie
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1 # Nieuwste stabiele versie
)
FetchContent_MakeAvailable(glm)

# Vulkan Memory Allocator (VMA)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.2.1 # Nieuwste stabiele versie
)
FetchContent_MakeAvailable(vma)

# stb_image
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# tinyobjloader
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        v1.0.6 # Latest stable version
)
FetchContent_MakeAvailable(tinyobjloader)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.2 # Latest stable version
)
FetchContent_MakeAvailable(spdlog)

# ResourcesVulkan
FetchContent_Declare(
    resourcesvulkan
    GIT_REPOSITORY https://github.com/KarelRoelandt/GP2_Sponza
    GIT_TAG        v1
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/models
)
FetchContent_MakeAvailable(resourcesvulkan)


set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_BUILD_DOCS OFF)
set(ASSIMP_BUILD_ZLIB ON)
    
# Add Assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG        v5.4.3 # Latest stable version
)
FetchContent_MakeAvailable(assimp)

set(GLM_INCLUDE_DIR ${glm_SOURCE_DIR})
set(VMA_INCLUDE_DIR ${vma_SOURCE_DIR}/include)
set(STB_INCLUDE_DIR ${stb_SOURCE_DIR})
set(TINYOBJLOADER_INCLUDE_DIR ${tinyobjloader_SOURCE_DIR})
set(SPDLOG_INCLUDE_DIR ${spdlog_SOURCE_DIR}/include)
set(ASSIMP_INCLUDE_DIR ${assimp_SOURCE_DIR}/include)

add_executable(VulkanProject 
 "VulkanProject.cpp" 
 "InstanceBuilder.h" 
 "Window.h" "Window.cpp" 
 "Instance.h" "Instance.cpp"
 "Surface.h" "Surface.cpp"
 "PhysicalDeviceBuilder.h" "PhysicalDeviceBuilder.cpp"
 "PhysicalDevice.h" "PhysicalDevice.cpp" 
 "Device.h" "DeviceBuilder.h" 
 "Device.cpp" "DeviceBuilder.cpp"
 "SwapChain.h" "SwapChain.cpp"
 "SwapChainBuilder.h" "SwapChainBuilder.cpp"
 "RenderPass.h" "RenderPass.cpp"
 "GraphicsPipeline.h" "GraphicsPipeline.cpp"
 "GraphicsPipelineBuilder.h" "GraphicsPipelineBuilder.cpp"
 "SynchronizationObjects.h" 
 "CommandPool.h" "CommandPool.cpp"
 "Buffer.h" "Buffer.cpp"
 "DescriptorManager.h" "DescriptorManager.cpp"
 "Image.h" "Image.cpp"
 "Model.h" "Model.cpp"
 "Texture.h" "Texture.cpp"
 "Renderer.h" "Renderer.cpp"
 "Camera.h" "Camera.cpp"
 "Material.h" "Material.cpp"
 "Frustum.h" "Frustum.cpp" 
 "ComputePipelineBuilder.h" "ComputePipelineBuilder.cpp" 
 "ComputePipeline.h" "ComputePipeline.cpp")

target_include_directories(VulkanProject PRIVATE 
    ${Vulkan_INCLUDE_DIRS} 
    ${GLM_INCLUDE_DIR}
    ${VMA_INCLUDE_DIR}
    ${STB_INCLUDE_DIR}
    ${TINYOBJLOADER_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIR}
    ${ASSIMP_INCLUDE_DIR}
)

target_link_libraries(VulkanProject PRIVATE 
    Vulkan::Vulkan 
    glfw 
    spdlog::spdlog
    assimp
)

set_target_properties(VulkanProject PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Compile shaders on every build
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

# Find all .vert and .frag files in the shader directory
file(GLOB SHADER_FILES "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.comp")

# Generate output paths for compiled shaders
set(COMPILED_SHADERS "")
foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(COMPILED_SHADER "${SHADER_OUT_DIR}/${SHADER_NAME}.spv")
    list(APPEND COMPILED_SHADERS ${COMPILED_SHADER})
    add_custom_command(
        OUTPUT ${COMPILED_SHADER}
        COMMAND C:/VulkanSDK/1.3.296.0/Bin/glslc.exe ${SHADER} -o ${COMPILED_SHADER}
        COMMENT "Compiling shader ${SHADER}..."
        VERBATIM
    )
endforeach()

add_custom_target(CompileShaders DEPENDS ${COMPILED_SHADERS})

# Ensure shaders are compiled before building the project
add_dependencies(VulkanProject CompileShaders)


# Copy .spv files to build directory
file(GLOB SPV_SHADERS "${SHADER_OUT_DIR}/*.spv")

foreach(SHADER ${SPV_SHADERS})
    add_custom_command(
        TARGET VulkanProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER} ${CMAKE_BINARY_DIR}/shaders/
        COMMENT "Copying ${SHADER} to build directory..."
    )
endforeach()

# Add default folder to build
set(DEFAULT_DIR "${CMAKE_SOURCE_DIR}/default")
set(DEFAULT_OUT_DIR "${CMAKE_BINARY_DIR}/default")
file(MAKE_DIRECTORY ${DEFAULT_OUT_DIR})

# Recursively gather all files and subdirectories in the default directory
file(GLOB_RECURSE DEFAULT_FILES "${DEFAULT_DIR}/*")

foreach(DEFAULT_FILE ${DEFAULT_FILES})
    # Compute the relative path of the file within the default directory
    file(RELATIVE_PATH REL_PATH "${DEFAULT_DIR}" "${DEFAULT_FILE}")
    # Construct the destination path in the build directory
    set(DEST_PATH "${DEFAULT_OUT_DIR}/${REL_PATH}")
    # Ensure the destination directory exists
    get_filename_component(DEST_DIR "${DEST_PATH}" DIRECTORY)
    file(MAKE_DIRECTORY "${DEST_DIR}")
    # Add a custom command to copy the file
    add_custom_command(
        TARGET VulkanProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DEFAULT_FILE}" "${DEST_PATH}"
        COMMENT "Copying default file ${DEFAULT_FILE} to build directory..."
    )
endforeach()


# Add models to build
set(MODEL_DIR "${CMAKE_SOURCE_DIR}/models")
set(MODEL_OUT_DIR "${CMAKE_BINARY_DIR}/models")
file(MAKE_DIRECTORY ${MODEL_OUT_DIR})

# Recursively gather all files and subdirectories in the models directory
file(GLOB_RECURSE MODEL_FILES "${MODEL_DIR}/*")

foreach(MODEL ${MODEL_FILES})
    # Compute the relative path of the file within the models directory
    file(RELATIVE_PATH REL_PATH "${MODEL_DIR}" "${MODEL}")
    # Construct the destination path in the build directory
    set(DEST_PATH "${MODEL_OUT_DIR}/${REL_PATH}")
    # Ensure the destination directory exists
    get_filename_component(DEST_DIR "${DEST_PATH}" DIRECTORY)
    file(MAKE_DIRECTORY "${DEST_DIR}")
    # Add a custom command to copy the file
    add_custom_command(
        TARGET VulkanProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MODEL}" "${DEST_PATH}"
        COMMENT "Copying model ${MODEL} to build directory..."
    )
endforeach()

