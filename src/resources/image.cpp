#include "astral/resources/image.hpp"
#include "astral/resources/buffer.hpp"
#include "astral/core/commands.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace astral {

Image::Image(Context* context, const ImageSpecs& specs)
    : m_context(context), m_specs(specs) {
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = specs.type;
    imageInfo.extent.width = specs.width;
    imageInfo.extent.height = specs.height;
    imageInfo.extent.depth = specs.depth;
    imageInfo.mipLevels = specs.mipLevels;
    imageInfo.arrayLayers = specs.arrayLayers;
    imageInfo.format = specs.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = specs.usage;
    imageInfo.samples = specs.samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (specs.viewType == VK_IMAGE_VIEW_TYPE_CUBE || specs.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = specs.memoryUsage;

    if (vmaCreateImage(m_context->getAllocator(), &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }

    createView();
}

Image::~Image() {
    vkDestroyImageView(m_context->getDevice(), m_view, nullptr);
    vmaDestroyImage(m_context->getAllocator(), m_image, m_allocation);
}

void Image::upload(const void* data, VkDeviceSize size) {
    // 1. Create staging buffer
    Buffer stagingBuffer(m_context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    stagingBuffer.upload(data, size);

    // 2. Transition layout to TRANSFER_DST_OPTIMAL
    CommandPool pool(m_context, m_context->getQueueFamilyIndices().graphicsFamily.value());
    auto cmd = pool.allocateBuffer();
    cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = m_specs.aspectFlags;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_specs.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_specs.arrayLayers;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd->getHandle(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // 3. Copy buffer to image
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = m_specs.aspectFlags;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = m_specs.arrayLayers;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_specs.width, m_specs.height, m_specs.depth};

    vkCmdCopyBufferToImage(
        cmd->getHandle(),
        stagingBuffer.getHandle(),
        m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    // 4. Transition layout to SHADER_READ_ONLY_OPTIMAL
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd->getHandle(),
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    cmd->end();
    cmd->submit(m_context->getGraphicsQueue());
    vkQueueWaitIdle(m_context->getGraphicsQueue());
}

void Image::createView() {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = m_specs.viewType;
    viewInfo.format = m_specs.format;
    viewInfo.subresourceRange.aspectMask = m_specs.aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_specs.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_specs.arrayLayers;

    if (vkCreateImageView(m_context->getDevice(), &viewInfo, nullptr, &m_view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }
}

} // namespace astral
