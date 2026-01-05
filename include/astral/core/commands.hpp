#pragma once

#include "astral/core/context.hpp"
#include <vulkan/vulkan.h>

namespace astral {

class CommandBuffer {
public:
    CommandBuffer(VkDevice device, VkCommandPool pool);
    ~CommandBuffer();

    void begin(VkCommandBufferUsageFlags flags = 0);
    void end();
    void submit(VkQueue queue, VkFence fence = VK_NULL_HANDLE);

    VkCommandBuffer getHandle() const { return m_handle; }

private:
    VkDevice m_device;
    VkCommandBuffer m_handle;
};

class CommandPool {
public:
    CommandPool(Context* context, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    ~CommandPool();

    std::unique_ptr<CommandBuffer> allocateBuffer();
    
    VkCommandPool getHandle() const { return m_handle; }

private:
    Context* m_context;
    VkCommandPool m_handle;
};

class ImmediateCommands {
public:
    ImmediateCommands(Context* context);
    ~ImmediateCommands();

    VkCommandBuffer getBuffer() const { return m_buffer; }

private:
    Context* m_context;
    VkCommandPool m_pool;
    VkCommandBuffer m_buffer;
};

} // namespace astral
