cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_DEFAULT_CMP0000 NEW)
project(Astral_Renderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(ASTRAL_BUILD_TESTS "Build tests" OFF)
option(ASTRAL_BUILD_EXAMPLES "Build example applications" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Dependencies using FetchContent
include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# fastgltf (Modern glTF 2.0 loader)
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG v0.8.0
)
FetchContent_MakeAvailable(fastgltf)

# Vulkan Memory Allocator (VMA)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.0.1
)
FetchContent_MakeAvailable(vma)

# stb (Single-file header-only libraries)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
)
FetchContent_MakeAvailable(imgui)

# Vulkan
find_package(Vulkan REQUIRED)

# Shaderc (Vulkan SDK component)
if(WIN32)
    add_library(shaderc_lib UNKNOWN IMPORTED)
    set_target_properties(shaderc_lib PROPERTIES
        IMPORTED_LOCATION         "$ENV{VULKAN_SDK}/Lib/shaderc_combined.lib"
        IMPORTED_LOCATION_RELEASE "$ENV{VULKAN_SDK}/Lib/shaderc_combined.lib"
        IMPORTED_LOCATION_DEBUG   "$ENV{VULKAN_SDK}/Lib/shaderc_combinedd.lib"
        INTERFACE_INCLUDE_DIRECTORIES "$ENV{VULKAN_SDK}/Include"
    )
    set(SHADERC_TARGET shaderc_lib)
else()
    find_package(shaderc REQUIRED)
    set(SHADERC_TARGET shaderc)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ImGui source files
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

# Astral Renderer Library
add_library(astral_renderer STATIC
    ${IMGUI_SOURCES}
)

target_include_directories(astral_renderer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${stb_SOURCE_DIR}
)

target_include_directories(astral_renderer PRIVATE
    src
)

target_link_libraries(astral_renderer
    PUBLIC
    Vulkan::Vulkan
    glfw
    glm::glm
    spdlog::spdlog
    fastgltf::fastgltf
    VulkanMemoryAllocator
    PRIVATE
    ${SHADERC_TARGET}
)

# Source files
set(ASTRAL_SOURCES
    src/core/context.cpp
    src/core/vma_implementation.cpp
    src/core/commands.cpp
    src/platform/window.cpp
    src/renderer/swapchain.cpp
    src/renderer/descriptor_manager.cpp
    src/renderer/pipeline.cpp
    src/renderer/render_graph.cpp
    src/renderer/sync.cpp
    src/renderer/scene_manager.cpp
    src/renderer/model.cpp
    src/renderer/gltf_loader.cpp
    src/renderer/camera.cpp
    src/renderer/compute_pipeline.cpp
    src/renderer/environment_manager.cpp
    src/renderer/ui_manager.cpp
    src/resources/buffer.cpp
    src/resources/image.cpp
    src/resources/shader.cpp
    src/resources/stb_image_impl.cpp
)

set(ASTRAL_HEADERS
    include/astral/astral.hpp
    include/astral/core/context.hpp
    include/astral/core/commands.hpp
    include/astral/platform/window.hpp
    include/astral/renderer/swapchain.hpp
    include/astral/renderer/descriptor_manager.hpp
    include/astral/renderer/pipeline.hpp
    include/astral/renderer/render_graph.hpp
    include/astral/renderer/sync.hpp
    include/astral/renderer/scene_data.hpp
    include/astral/renderer/scene_manager.hpp
    include/astral/renderer/model.hpp
    include/astral/renderer/gltf_loader.hpp
    include/astral/renderer/camera.hpp
    include/astral/renderer/compute_pipeline.hpp
    include/astral/renderer/environment_manager.hpp
    include/astral/renderer/ui_manager.hpp
    include/astral/resources/buffer.hpp
    include/astral/resources/image.hpp
    include/astral/resources/shader.hpp
)

# Placeholder files to make CMake happy for now
# We will create these in the next step
target_sources(astral_renderer PRIVATE ${ASTRAL_SOURCES} ${ASTRAL_HEADERS})

# Example Application
if(ASTRAL_BUILD_EXAMPLES)
    add_executable(AstralSandbox src/main.cpp)
    target_link_libraries(AstralSandbox PRIVATE astral_renderer)
endif()
