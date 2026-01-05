#pragma once

#include "astral/core/context.hpp"
#include <vk_mem_alloc.h>

namespace astral {

class Buffer {
public:
    Buffer(Context* context, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags flags = 0);
    ~Buffer();

    // Disable copying
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    void map(void** data);
    void unmap();
    void upload(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    VkBuffer getHandle() const { return m_buffer; }
    VmaAllocation getAllocation() const { return m_allocation; }
    VkDeviceSize getSize() const { return m_size; }

private:
    Context* m_context;
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    VkDeviceSize m_size;
    void* m_mappedData = nullptr;
};

} // namespace astral
