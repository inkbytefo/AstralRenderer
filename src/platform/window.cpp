#include "astral/platform/window.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace astral {

Window::Window(const WindowSpecs& specs) : m_specs(specs) {
    if (!glfwInit()) {
        spdlog::error("Failed to initialize GLFW");
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(m_specs.width, m_specs.height, m_specs.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        spdlog::error("Failed to create GLFW window");
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    
    spdlog::info("Window created: {} ({}x{})", m_specs.title, m_specs.width, m_specs.height);
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
    spdlog::info("Window destroyed");
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

std::vector<const char*> Window::getRequiredExtensions() const {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return extensions;
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    app->m_specs.width = static_cast<uint32_t>(width);
    app->m_specs.height = static_cast<uint32_t>(height);
}

} // namespace astral
