#include "astral/core/commands.hpp"
#include <stdexcept>

namespace astral {

// CommandBuffer
CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool pool) : m_device(device) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_handle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer!");
    }
}

CommandBuffer::~CommandBuffer() {
    // Command buffers are automatically freed when the pool is destroyed
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    if (vkBeginCommandBuffer(m_handle, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
}

void CommandBuffer::end() {
    if (vkEndCommandBuffer(m_handle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void CommandBuffer::submit(VkQueue queue, VkFence fence) {
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_handle;

    if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit command buffer!");
    }
}

// CommandPool
CommandPool::CommandPool(Context* context, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
    : m_context(context) {
    
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = flags;

    if (vkCreateCommandPool(m_context->getDevice(), &poolInfo, nullptr, &m_handle) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(m_context->getDevice(), m_handle, nullptr);
}

std::unique_ptr<CommandBuffer> CommandPool::allocateBuffer() {
    return std::make_unique<CommandBuffer>(m_context->getDevice(), m_handle);
}

// ImmediateCommands
ImmediateCommands::ImmediateCommands(Context* context) : m_context(context) {
    VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = m_context->getQueueFamilyIndices().graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    vkCreateCommandPool(m_context->getDevice(), &poolInfo, nullptr, &m_pool);

    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_pool;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(m_context->getDevice(), &allocInfo, &m_buffer);

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_buffer, &beginInfo);
}

ImmediateCommands::~ImmediateCommands() {
    vkEndCommandBuffer(m_buffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_buffer;

    VkQueue graphicsQueue = m_context->getGraphicsQueue();
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkDestroyCommandPool(m_context->getDevice(), m_pool, nullptr);
}

} // namespace astral
