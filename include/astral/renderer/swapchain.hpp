#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace astral {

class Context;
class Window;

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain {
public:
    Swapchain(Context* context, Window* window);
    ~Swapchain();

    VkSwapchainKHR getHandle() const { return m_swapchain; }
    VkFormat getImageFormat() const { return m_imageFormat; }
    VkExtent2D getExtent() const { return m_extent; }
    const std::vector<VkImage>& getImages() const { return m_images; }
    const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }

    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }

private:
    void create();
    void createImageViews();

    SwapchainSupportDetails querySupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window* window);

    Context* m_context;
    Window* m_window;

    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    VkFormat m_imageFormat;
    VkExtent2D m_extent;
};

} // namespace astral
