#include "astral/renderer/sync.hpp"
#include <stdexcept>

namespace astral {

FrameSync::FrameSync(Context* context, uint32_t maxFramesInFlight)
    : m_context(context), m_maxFramesInFlight(maxFramesInFlight) {
    
    m_imageAvailableSemaphores.resize(maxFramesInFlight);
    m_renderFinishedSemaphores.resize(maxFramesInFlight);
    m_inFlightFences.resize(maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(m_context->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_context->getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_context->getDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

FrameSync::~FrameSync() {
    for (uint32_t i = 0; i < m_maxFramesInFlight; i++) {
        vkDestroySemaphore(m_context->getDevice(), m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_context->getDevice(), m_renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_context->getDevice(), m_inFlightFences[i], nullptr);
    }
}

void FrameSync::waitForFrame(uint32_t frameIndex) {
    vkWaitForFences(m_context->getDevice(), 1, &m_inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
}

void FrameSync::resetFence(uint32_t frameIndex) {
    vkResetFences(m_context->getDevice(), 1, &m_inFlightFences[frameIndex]);
}

} // namespace astral
