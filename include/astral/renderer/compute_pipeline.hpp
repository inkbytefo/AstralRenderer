#pragma once

#include "astral/core/context.hpp"
#include "astral/resources/shader.hpp"
#include <vulkan/vulkan.h>
#include <memory>

namespace astral {

struct ComputePipelineSpecs {
    std::shared_ptr<Shader> computeShader;
    VkPipelineLayout layout;
};

class ComputePipeline {
public:
    ComputePipeline(Context* context, const ComputePipelineSpecs& specs);
    ~ComputePipeline();

    VkPipeline getHandle() const { return m_pipeline; }

private:
    Context* m_context;
    VkPipeline m_pipeline;
};

} // namespace astral
