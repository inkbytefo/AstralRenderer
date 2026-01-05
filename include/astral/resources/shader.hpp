#pragma once

#include "astral/core/context.hpp"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <shaderc/shaderc.hpp>

namespace astral {

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute
};

class Shader {
public:
    Shader(Context* context, const std::string& source, ShaderStage stage, const std::string& name = "shader");
    ~Shader();

    VkShaderModule getModule() const { return m_module; }
    VkPipelineShaderStageCreateInfo getStageInfo() const;

private:
    Context* m_context;
    VkShaderModule m_module;
    ShaderStage m_stage;
    std::string m_name;

    std::vector<uint32_t> compile(const std::string& source, ShaderStage stage);
};

} // namespace astral
