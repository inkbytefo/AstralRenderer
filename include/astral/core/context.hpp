#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <memory>
#include <optional>

namespace astral {

class Window; // Forward declaration
class DescriptorManager;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value() && transferFamily.has_value();
    }
};

class Context {
public:
    Context(Window* window);
    ~Context();

    VkInstance getInstance() const { return m_instance; }
    VkDevice getDevice() const { return m_device; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkSurfaceKHR getSurface() const { return m_surface; }
    VmaAllocator getAllocator() const { return m_allocator; }
    
    QueueFamilyIndices getQueueFamilyIndices() const { return m_indices; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }

    DescriptorManager& getDescriptorManager() { return *m_descriptorManager; }
    Window& getWindow() { return *m_window; }

private:
    void createInstance(const std::vector<const char*>& requiredExtensions);
    void setupDebugMessenger();
    void createSurface(Window* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createAllocator();

    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    Window* m_window;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device;
    VmaAllocator m_allocator;

    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkQueue m_computeQueue;
    VkQueue m_transferQueue;

    QueueFamilyIndices m_indices;

    std::unique_ptr<DescriptorManager> m_descriptorManager;

    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool m_enableValidationLayers = false;
#else
    const bool m_enableValidationLayers = true;
#endif
};

} // namespace astral
