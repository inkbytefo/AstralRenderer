#include "astral/resources/buffer.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace astral {

Buffer::Buffer(Context* context, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags)
    : m_context(context), m_size(size) {
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;

    if (vmaCreateBuffer(m_context->getAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }
}

Buffer::~Buffer() {
    if (m_mappedData) {
        unmap();
    }
    vmaDestroyBuffer(m_context->getAllocator(), m_buffer, m_allocation);
}

void Buffer::map(void** data) {
    if (vmaMapMemory(m_context->getAllocator(), m_allocation, data) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map buffer memory!");
    }
    m_mappedData = *data;
}

void Buffer::unmap() {
    vmaUnmapMemory(m_context->getAllocator(), m_allocation);
    m_mappedData = nullptr;
}

void Buffer::upload(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (size + offset > m_size) {
        throw std::runtime_error("Upload size + offset exceeds buffer size!");
    }

    void* mapped;
    map(&mapped);
    memcpy(static_cast<char*>(mapped) + offset, data, size);
    unmap();
}

} // namespace astral
