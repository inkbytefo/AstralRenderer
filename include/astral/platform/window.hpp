#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

namespace astral {

struct WindowSpecs {
    uint32_t width = 1280;
    uint32_t height = 720;
    std::string title = "Astral Renderer";
};

class Window {
public:
    Window(const WindowSpecs& specs);
    ~Window();

    bool shouldClose() const;
    void pollEvents();
    
    GLFWwindow* getNativeWindow() const { return m_window; }
    uint32_t getWidth() const { return m_specs.width; }
    uint32_t getHeight() const { return m_specs.height; }

    std::vector<const char*> getRequiredExtensions() const;

private:
    GLFWwindow* m_window;
    WindowSpecs m_specs;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

} // namespace astral
