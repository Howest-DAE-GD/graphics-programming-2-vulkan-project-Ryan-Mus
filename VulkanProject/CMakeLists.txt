cmake_minimum_required(VERSION 3.16)

project(VulkanProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND OR NOT Vulkan_INCLUDE_DIRS OR NOT Vulkan_LIBRARIES)
    message(FATAL_ERROR "❌ Vulkan SDK niet gevonden!")
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

set(GLM_INCLUDE_DIR ${glm_SOURCE_DIR})

add_executable(VulkanProject 
    "VulkanProject.cpp" 
)

target_include_directories(VulkanProject PRIVATE 
    ${Vulkan_INCLUDE_DIRS} 
    ${GLM_INCLUDE_DIR}
)

target_link_libraries(VulkanProject PRIVATE 
    Vulkan::Vulkan 
    glfw 
)

set_target_properties(VulkanProject PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)