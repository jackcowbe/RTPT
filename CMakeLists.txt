cmake_minimum_required(VERSION 3.20)
project(RTPT CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(RTPT
    src/main.cpp
    src/window.cpp
    src/vulkan_instance.cpp
    src/vulkan_surface.cpp
    src/vulkan_physicaldevice.cpp
)

target_include_directories(RTPT PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    src
)

target_link_libraries(RTPT PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
)

if (MSVC)
    target_compile_options(RTPT PRIVATE /W4 /utf-8)
endif()
