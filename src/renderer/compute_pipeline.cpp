#include "astral/renderer/compute_pipeline.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace astral {

ComputePipeline::ComputePipeline(Context* context, const ComputePipelineSpecs& specs)
    : m_context(context) {
    
    VkComputePipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = specs.computeShader->getModule();
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = specs.layout;

    if (vkCreateComputePipelines(m_context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline!");
    }

    spdlog::debug("Compute pipeline created successfully");
}

ComputePipeline::~ComputePipeline() {
    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_context->getDevice(), m_pipeline, nullptr);
    }
}

} // namespace astral
