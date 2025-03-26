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

set(GLM_INCLUDE_DIR ${glm_SOURCE_DIR})
set(VMA_INCLUDE_DIR ${vma_SOURCE_DIR}/include)
set(STB_INCLUDE_DIR ${stb_SOURCE_DIR})
set(TINYOBJLOADER_INCLUDE_DIR ${tinyobjloader_SOURCE_DIR})

add_executable(VulkanProject 
    "VulkanProject.cpp" 
)

target_include_directories(VulkanProject PRIVATE 
    ${Vulkan_INCLUDE_DIRS} 
    ${GLM_INCLUDE_DIR}
    ${VMA_INCLUDE_DIR}
    ${STB_INCLUDE_DIR}
    ${TINYOBJLOADER_INCLUDE_DIR}
)

target_link_libraries(VulkanProject PRIVATE 
    Vulkan::Vulkan 
    glfw 
)

set_target_properties(VulkanProject PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Compileer shaders bij iedere build
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

add_custom_command(
    OUTPUT ${SHADER_OUT_DIR}/vert.spv ${SHADER_OUT_DIR}/frag.spv
    COMMAND C:/VulkanSDK/1.3.296.0/Bin/glslc.exe ${SHADER_DIR}/shader.vert -o ${SHADER_OUT_DIR}/vert.spv
    COMMAND C:/VulkanSDK/1.3.296.0/Bin/glslc.exe ${SHADER_DIR}/shader.frag -o ${SHADER_OUT_DIR}/frag.spv
    COMMENT "Compiling shaders..."
    VERBATIM
)

add_custom_target(CompileShaders DEPENDS ${SHADER_OUT_DIR}/vert.spv ${SHADER_OUT_DIR}/frag.spv)

# Zorg ervoor dat de shaders worden gecompileerd voordat het project wordt gebouwd
add_dependencies(VulkanProject CompileShaders)

# Kopieer alleen de .spv bestanden naar de build-directory
file(GLOB SPV_SHADERS "${SHADER_OUT_DIR}/*.spv")

foreach(SHADER ${SPV_SHADERS})
    add_custom_command(
        TARGET VulkanProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SHADER} ${CMAKE_BINARY_DIR}/shaders/
        COMMENT "Copying from ${SHADER} to build-directory..."
    )
endforeach()

# Add textures to build
set(TEXTURE_DIR "${CMAKE_SOURCE_DIR}/Textures")
set(TEXTURE_OUT_DIR "${CMAKE_BINARY_DIR}/Textures")
file(MAKE_DIRECTORY ${TEXTURE_OUT_DIR})

file(GLOB TEXTURE_FILES "${TEXTURE_DIR}/*")

foreach(TEXTURE ${TEXTURE_FILES})
    add_custom_command(
        TARGET VulkanProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TEXTURE} ${TEXTURE_OUT_DIR}/
        COMMENT "Copying texture ${TEXTURE} to build-directory..."
    )
endforeach()

# Add models to build
set(MODEL_DIR "${CMAKE_SOURCE_DIR}/models")
set(MODEL_OUT_DIR "${CMAKE_BINARY_DIR}/models")
file(MAKE_DIRECTORY ${MODEL_OUT_DIR})

file(GLOB MODEL_FILES "${MODEL_DIR}/*")

foreach(MODEL ${MODEL_FILES})
    add_custom_command(
        TARGET VulkanProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MODEL} ${MODEL_OUT_DIR}/
        COMMENT "Copying model ${MODEL} to build-directory..."
    )
endforeach()
