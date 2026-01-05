#include "astral/resources/shader.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace astral {

Shader::Shader(Context* context, const std::string& source, ShaderStage stage, const std::string& name)
    : m_context(context), m_stage(stage), m_name(name) {
    
    auto spirv = compile(source, stage);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    if (vkCreateShaderModule(m_context->getDevice(), &createInfo, nullptr, &m_module) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module: " + name);
    }
}

Shader::~Shader() {
    vkDestroyShaderModule(m_context->getDevice(), m_module, nullptr);
}

VkPipelineShaderStageCreateInfo Shader::getStageInfo() const {
    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    
    switch (m_stage) {
        case ShaderStage::Vertex: stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; break;
        case ShaderStage::Fragment: stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
        case ShaderStage::Compute: stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT; break;
    }

    stageInfo.module = m_module;
    stageInfo.pName = "main";
    return stageInfo;
}

std::vector<uint32_t> Shader::compile(const std::string& source, ShaderStage stage) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    shaderc_shader_kind kind;
    switch (stage) {
        case ShaderStage::Vertex: kind = shaderc_glsl_vertex_shader; break;
        case ShaderStage::Fragment: kind = shaderc_glsl_fragment_shader; break;
        case ShaderStage::Compute: kind = shaderc_glsl_compute_shader; break;
    }

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, kind, m_name.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        spdlog::error("Shader compilation error ({}): {}", m_name, result.GetErrorMessage());
        throw std::runtime_error("Failed to compile shader: " + m_name);
    }

    return {result.cbegin(), result.cend()};
}

} // namespace astral
