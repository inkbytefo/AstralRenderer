#pragma once

#include "astral/core/context.hpp"
#include <vulkan/vulkan.h>
#include <functional>
#include <string>
#include <vector>
#include <map>

namespace astral {

struct RenderPassResource {
    std::string name;
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format;
    uint32_t width;
    uint32_t height;
    VkClearValue clearValue;
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bool isExternal = false;
};

using RenderPassExecuteCallback = std::function<void(VkCommandBuffer)>;

struct RenderPassNode {
    std::string name;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    RenderPassExecuteCallback execute;
    bool clearOutputs = true; // Added to support UI overlays
};

class RenderGraph {
public:
    RenderGraph(Context* context);
    ~RenderGraph();

    void addPass(const std::string& name, 
                 const std::vector<std::string>& inputs, 
                 const std::vector<std::string>& outputs, 
                 RenderPassExecuteCallback execute,
                 bool clearOutputs = true);

    void addExternalResource(const std::string& name, VkImage image, VkImageView view, VkFormat format, uint32_t width, uint32_t height, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
    void setResourceClearValue(const std::string& name, VkClearValue clearValue);

    void execute(VkCommandBuffer cmd, VkExtent2D extent);
    void clear();

private:
    Context* m_context;
    std::vector<RenderPassNode> m_passes;
    std::map<std::string, RenderPassResource> m_resources;
    std::map<VkImage, VkImageLayout> m_imageLayouts;
};

} // namespace astral
