#include <iostream>
#include <random>
#include <vector>
#include <array>
#include <spdlog/spdlog.h>
#include "astral/astral.hpp"
#include "astral/renderer/environment_manager.hpp"
#include "astral/renderer/ui_manager.hpp"
#include "astral/renderer/compute_pipeline.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>
#include <fstream>
#include <sstream>
#include <filesystem>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting Astral Renderer Sandbox...");

    try {
        astral::WindowSpecs specs;
        specs.title = "Astral Renderer - glTF PBR Sandbox";
        specs.width = 1600;
        specs.height = 900;

        astral::Window window(specs);
        
        astral::Context context(&window);
        astral::Swapchain swapchain(&context, &window);
        astral::FrameSync sync(&context, 1);
        astral::CommandPool commandPool(&context, context.getQueueFamilyIndices().graphicsFamily.value());
        auto cmd = commandPool.allocateBuffer();

        astral::SceneManager sceneManager(&context);
        astral::EnvironmentManager envManager(&context);
        
        // Try to load a skybox if exists, otherwise it will stay default
        std::string hdrPath = "assets/textures/skybox.hdr";
        if (std::filesystem::exists(hdrPath)) {
            envManager.loadHDR(hdrPath);
        } else {
            spdlog::warn("Skybox HDR not found at: {}. IBL will be disabled.", hdrPath);
        }
        astral::MaterialMetadata defaultMat;
        defaultMat.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        defaultMat.metallicFactor = 0.5f;
        defaultMat.roughnessFactor = 0.5f;
        defaultMat.baseColorTextureIndex = -1;
        defaultMat.metallicRoughnessTextureIndex = -1;
        defaultMat.normalTextureIndex = -1;
        defaultMat.occlusionTextureIndex = -1;
        defaultMat.emissiveTextureIndex = -1;
        defaultMat.alphaCutoff = 0.5f;
        sceneManager.addMaterial(defaultMat);

        astral::GltfLoader loader(&context);
        astral::Camera camera;
        camera.setPerspective(45.0f, (float)specs.width / (float)specs.height, 0.1f, 1000.0f);
        camera.setPosition(glm::vec3(0.0f, 0.0f, 5.0f)); 

        // Set up input callbacks
        glfwSetWindowUserPointer(window.getNativeWindow(), &camera);
        
        glfwSetCursorPosCallback(window.getNativeWindow(), [](GLFWwindow* window, double xpos, double ypos) {
            static double lastX = 800.0, lastY = 450.0;
            static bool firstMouse = true;
            if (firstMouse) {
                lastX = xpos;
                lastY = ypos;
                firstMouse = false;
            }
            double xoffset = xpos - lastX;
            double yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
            lastX = xpos;
            lastY = ypos;

            auto cam = static_cast<astral::Camera*>(glfwGetWindowUserPointer(window));
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                cam->processMouse(static_cast<float>(xoffset), static_cast<float>(yoffset));
            }
        });

        glfwSetInputMode(window.getNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        // Load Model
        auto model = loader.loadFromFile("assets/models/damaged_helmet/scene.gltf", &sceneManager);
        
        if (!model) {
            spdlog::warn("Model not found at assets/models/damaged_helmet/scene.gltf, creating fallback...");
            // Fallback could be a simple cube or just empty
        }

        astral::RenderGraph graph(&context);

        // Depth Image
        astral::ImageSpecs depthSpecs;
        depthSpecs.width = specs.width;
        depthSpecs.height = specs.height;
        depthSpecs.format = VK_FORMAT_D32_SFLOAT;
        depthSpecs.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        depthSpecs.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        auto depthImage = std::make_unique<astral::Image>(&context, depthSpecs);

        // Load PBR Shaders
        auto vertShader = std::make_shared<astral::Shader>(&context, readFile("shaders/pbr.vert"), astral::ShaderStage::Vertex, "PBRVert");
        auto fragShader = std::make_shared<astral::Shader>(&context, readFile("shaders/pbr.frag"), astral::ShaderStage::Fragment, "PBRFrag");

        // Pipeline Layout with Push Constants and Bindless Descriptor Set
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(uint32_t) * 4; // sceneDataIndex + instanceBufferIndex + materialBufferIndex + padding/cascadeIndex

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        
        VkDescriptorSetLayout setLayouts[] = { context.getDescriptorManager().getLayout() };
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = setLayouts;

        VkPipelineLayout pipelineLayout;
        if (vkCreatePipelineLayout(context.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        // Graphics Pipeline
        astral::PipelineSpecs pipelineSpecs;
        pipelineSpecs.vertexShader = vertShader;
        pipelineSpecs.fragmentShader = fragShader;
        pipelineSpecs.layout = pipelineLayout;
        pipelineSpecs.colorFormats = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT }; // HDR + Normal
        pipelineSpecs.depthFormat = VK_FORMAT_D32_SFLOAT; 
        pipelineSpecs.cullMode = VK_CULL_MODE_NONE; // Disable culling to see the model even if normals are flipped

        auto vertexBinding = astral::Vertex::getBindingDescription();
        auto vertexAttrs = astral::Vertex::getAttributeDescriptions();
        pipelineSpecs.vertexBindings.push_back(vertexBinding);
        pipelineSpecs.vertexAttributes = vertexAttrs;
        
        auto pipeline = std::make_unique<astral::GraphicsPipeline>(&context, pipelineSpecs);

        // --- HDR and Post-Processing Setup ---
        auto postVertShader = std::make_shared<astral::Shader>(&context, readFile("shaders/post_process.vert"), astral::ShaderStage::Vertex, "PostVert");
        
        astral::ImageSpecs hdrSpecs;
        hdrSpecs.width = specs.width;
        hdrSpecs.height = specs.height;
        hdrSpecs.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        hdrSpecs.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        hdrSpecs.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        auto hdrImage = std::make_unique<astral::Image>(&context, hdrSpecs);

        // Normal Attachment for SSAO
        astral::ImageSpecs normalSpecs = hdrSpecs;
        normalSpecs.format = VK_FORMAT_R16G16B16A16_SFLOAT; // Or R8G8B8A8_SNORM
        auto normalImage = std::make_unique<astral::Image>(&context, normalSpecs);

        VkSampler hdrSampler;
        VkSamplerCreateInfo hdrSamplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        hdrSamplerInfo.magFilter = VK_FILTER_LINEAR;
        hdrSamplerInfo.minFilter = VK_FILTER_LINEAR;
        hdrSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        hdrSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        hdrSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        hdrSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(context.getDevice(), &hdrSamplerInfo, nullptr, &hdrSampler);

        uint32_t hdrTextureIndex = context.getDescriptorManager().registerImage(hdrImage->getView(), hdrSampler);
        uint32_t normalTextureIndex = context.getDescriptorManager().registerImage(normalImage->getView(), hdrSampler);
        uint32_t depthTextureIndex = context.getDescriptorManager().registerImage(depthImage->getView(), hdrSampler);

        // --- SSAO Setup ---
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
        std::default_random_engine generator;
        const uint32_t ssaoKernelSize = 32;
        std::vector<glm::vec4> ssaoKernel;
        for (uint32_t i = 0; i < ssaoKernelSize; ++i) {
            glm::vec3 sample(
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator)
            );
            sample = glm::normalize(sample);
            sample *= randomFloats(generator);
            float scale = (float)i / (float)ssaoKernelSize;
            scale = glm::mix(0.1f, 1.0f, scale * scale);
            sample *= scale;
            ssaoKernel.push_back(glm::vec4(sample, 0.0f));
        }

        auto ssaoKernelBuffer = std::make_unique<astral::Buffer>(&context, ssaoKernel.size() * sizeof(glm::vec4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        ssaoKernelBuffer->upload(ssaoKernel.data(), ssaoKernel.size() * sizeof(glm::vec4));
        uint32_t ssaoKernelBufferIndex = context.getDescriptorManager().registerBuffer(ssaoKernelBuffer->getHandle(), 0, ssaoKernelBuffer->getSize(), 2); // Binding 2 for SSAO Kernel (Material binding unused here)

        std::vector<glm::vec4> ssaoNoise;
        for (uint32_t i = 0; i < 16; ++i) {
            glm::vec4 noise(
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0,
                0.0f,
                0.0f
            );
            ssaoNoise.push_back(noise);
        }

        astral::ImageSpecs noiseSpecs;
        noiseSpecs.width = 4;
        noiseSpecs.height = 4;
        noiseSpecs.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        noiseSpecs.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        auto noiseImage = std::make_unique<astral::Image>(&context, noiseSpecs);
        noiseImage->upload(ssaoNoise.data(), ssaoNoise.size() * sizeof(glm::vec4));

        VkSampler noiseSampler;
        VkSamplerCreateInfo noiseSamplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        noiseSamplerInfo.magFilter = VK_FILTER_NEAREST;
        noiseSamplerInfo.minFilter = VK_FILTER_NEAREST;
        noiseSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        noiseSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vkCreateSampler(context.getDevice(), &noiseSamplerInfo, nullptr, &noiseSampler);
        uint32_t noiseTextureIndex = context.getDescriptorManager().registerImage(noiseImage->getView(), noiseSampler);

        astral::ImageSpecs ssaoSpecs;
        ssaoSpecs.width = specs.width;
        ssaoSpecs.height = specs.height;
        ssaoSpecs.format = VK_FORMAT_R8_UNORM;
        ssaoSpecs.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        auto ssaoImage = std::make_unique<astral::Image>(&context, ssaoSpecs);
        auto ssaoBlurImage = std::make_unique<astral::Image>(&context, ssaoSpecs);

        uint32_t ssaoTextureIndex = context.getDescriptorManager().registerImage(ssaoImage->getView(), hdrSampler);
        uint32_t ssaoBlurTextureIndex = context.getDescriptorManager().registerImage(ssaoBlurImage->getView(), hdrSampler);

        auto ssaoFragShader = std::make_shared<astral::Shader>(&context, readFile("shaders/ssao.frag"), astral::ShaderStage::Fragment, "SSAOFrag");
        auto ssaoBlurFragShader = std::make_shared<astral::Shader>(&context, readFile("shaders/ssao_blur.frag"), astral::ShaderStage::Fragment, "SSAOBlurFrag");

        VkPushConstantRange ssaoPushRange = {};
        ssaoPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        ssaoPushRange.offset = 0;
        ssaoPushRange.size = sizeof(int) * 4 + sizeof(float) * 2; // normalIdx, depthIdx, noiseIdx, kernelIdx, radius, bias

        VkPipelineLayoutCreateInfo ssaoLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        ssaoLayoutInfo.pushConstantRangeCount = 1;
        ssaoLayoutInfo.pPushConstantRanges = &ssaoPushRange;
        ssaoLayoutInfo.setLayoutCount = 1;
        ssaoLayoutInfo.pSetLayouts = setLayouts;

        VkPipelineLayout ssaoLayout;
        vkCreatePipelineLayout(context.getDevice(), &ssaoLayoutInfo, nullptr, &ssaoLayout);

        astral::PipelineSpecs ssaoPipelineSpecs;
        ssaoPipelineSpecs.vertexShader = postVertShader;
        ssaoPipelineSpecs.fragmentShader = ssaoFragShader;
        ssaoPipelineSpecs.layout = ssaoLayout;
        ssaoPipelineSpecs.colorFormats = { VK_FORMAT_R8_UNORM };
        ssaoPipelineSpecs.depthFormat = VK_FORMAT_UNDEFINED;
        ssaoPipelineSpecs.depthTest = false;
        auto ssaoPipeline = std::make_unique<astral::GraphicsPipeline>(&context, ssaoPipelineSpecs);

        VkPushConstantRange ssaoBlurPushRange = {};
        ssaoBlurPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        ssaoBlurPushRange.offset = 0;
        ssaoBlurPushRange.size = sizeof(int);

        VkPipelineLayoutCreateInfo ssaoBlurLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        ssaoBlurLayoutInfo.pushConstantRangeCount = 1;
        ssaoBlurLayoutInfo.pPushConstantRanges = &ssaoBlurPushRange;
        ssaoBlurLayoutInfo.setLayoutCount = 1;
        ssaoBlurLayoutInfo.pSetLayouts = setLayouts;

        VkPipelineLayout ssaoBlurLayout;
        vkCreatePipelineLayout(context.getDevice(), &ssaoBlurLayoutInfo, nullptr, &ssaoBlurLayout);

        astral::PipelineSpecs ssaoBlurPipelineSpecs;
        ssaoBlurPipelineSpecs.vertexShader = postVertShader;
        ssaoBlurPipelineSpecs.fragmentShader = ssaoBlurFragShader;
        ssaoBlurPipelineSpecs.layout = ssaoBlurLayout;
        ssaoBlurPipelineSpecs.colorFormats = { VK_FORMAT_R8_UNORM };
        ssaoBlurPipelineSpecs.depthFormat = VK_FORMAT_UNDEFINED;
        ssaoBlurPipelineSpecs.depthTest = false;
        auto ssaoBlurPipeline = std::make_unique<astral::GraphicsPipeline>(&context, ssaoBlurPipelineSpecs);

        // --- End SSAO Setup ---

        auto compositeFragShader = std::make_shared<astral::Shader>(&context, readFile("shaders/composite.frag"), astral::ShaderStage::Fragment, "CompositeFrag");

        VkPushConstantRange compositePushRange = {};
        compositePushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        compositePushRange.offset = 0;
        compositePushRange.size = sizeof(int) * 3 + sizeof(float) * 2 + sizeof(int); // hdrIdx, bloomIdx, ssaoIdx, exposure, bloomStrength, enableSSAO

        VkPipelineLayoutCreateInfo compositeLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        compositeLayoutInfo.pushConstantRangeCount = 1;
        compositeLayoutInfo.pPushConstantRanges = &compositePushRange;
        compositeLayoutInfo.setLayoutCount = 1;
        compositeLayoutInfo.pSetLayouts = setLayouts;

        VkPipelineLayout compositeLayout;
        vkCreatePipelineLayout(context.getDevice(), &compositeLayoutInfo, nullptr, &compositeLayout);

        astral::PipelineSpecs compositeSpecs;
        compositeSpecs.vertexShader = postVertShader;
        compositeSpecs.fragmentShader = compositeFragShader;
        compositeSpecs.layout = compositeLayout;
        compositeSpecs.colorFormats = { VK_FORMAT_R8G8B8A8_UNORM };
        compositeSpecs.depthFormat = VK_FORMAT_UNDEFINED;
        compositeSpecs.depthTest = false;
        compositeSpecs.depthWrite = false;
        compositeSpecs.cullMode = VK_CULL_MODE_NONE;
        
        auto compositePipeline = std::make_unique<astral::GraphicsPipeline>(&context, compositeSpecs);

        // --- Bloom Setup ---
        astral::ImageSpecs bloomSpecs;
        bloomSpecs.width = specs.width / 4;
        bloomSpecs.height = specs.height / 4;
        bloomSpecs.format = VK_FORMAT_R16G16B16A16_SFLOAT;
        bloomSpecs.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        bloomSpecs.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        auto bloomImage = std::make_unique<astral::Image>(&context, bloomSpecs);
        auto bloomBlurImage = std::make_unique<astral::Image>(&context, bloomSpecs);

        uint32_t bloomTextureIndex = context.getDescriptorManager().registerImage(bloomImage->getView(), hdrSampler);
        uint32_t bloomBlurTextureIndex = context.getDescriptorManager().registerImage(bloomBlurImage->getView(), hdrSampler);

        auto bloomFragShader = std::make_shared<astral::Shader>(&context, readFile("shaders/bloom.frag"), astral::ShaderStage::Fragment, "BloomFrag");

        VkPushConstantRange bloomPushRange = {};
        bloomPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bloomPushRange.offset = 0;
        bloomPushRange.size = sizeof(int) * 2 + sizeof(float) * 2; // inputIdx, mode, threshold, softness

        VkPipelineLayoutCreateInfo bloomLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        bloomLayoutInfo.pushConstantRangeCount = 1;
        bloomLayoutInfo.pPushConstantRanges = &bloomPushRange;
        bloomLayoutInfo.setLayoutCount = 1;
        bloomLayoutInfo.pSetLayouts = setLayouts;

        VkPipelineLayout bloomLayout;
        vkCreatePipelineLayout(context.getDevice(), &bloomLayoutInfo, nullptr, &bloomLayout);

        astral::PipelineSpecs bloomPipelineSpecs;
        bloomPipelineSpecs.vertexShader = postVertShader;
        bloomPipelineSpecs.fragmentShader = bloomFragShader;
        bloomPipelineSpecs.layout = bloomLayout;
        bloomPipelineSpecs.colorFormats = { VK_FORMAT_R16G16B16A16_SFLOAT };
        bloomPipelineSpecs.depthFormat = VK_FORMAT_UNDEFINED;
        bloomPipelineSpecs.depthTest = false;
        bloomPipelineSpecs.depthWrite = false;
        bloomPipelineSpecs.cullMode = VK_CULL_MODE_NONE;

        auto bloomPipeline = std::make_unique<astral::GraphicsPipeline>(&context, bloomPipelineSpecs);

        // --- FXAA Setup ---
        auto fxaaFragShader = std::make_shared<astral::Shader>(&context, readFile("shaders/fxaa.frag"), astral::ShaderStage::Fragment, "FXAAFrag");

        VkPushConstantRange fxaaPushRange = {};
        fxaaPushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        fxaaPushRange.offset = 0;
        fxaaPushRange.size = sizeof(int) + sizeof(glm::vec2);

        VkDescriptorSetLayout fxaaSetLayouts[] = {context.getDescriptorManager().getLayout()};
        VkPipelineLayoutCreateInfo fxaaLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        fxaaLayoutInfo.pushConstantRangeCount = 1;
        fxaaLayoutInfo.pPushConstantRanges = &fxaaPushRange;
        fxaaLayoutInfo.setLayoutCount = 1;
        fxaaLayoutInfo.pSetLayouts = fxaaSetLayouts;

        VkPipelineLayout fxaaLayout;
        if (vkCreatePipelineLayout(context.getDevice(), &fxaaLayoutInfo, nullptr, &fxaaLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create FXAA pipeline layout!");
        }

        astral::PipelineSpecs fxaaPipelineSpecs;
        fxaaPipelineSpecs.vertexShader = postVertShader;
        fxaaPipelineSpecs.fragmentShader = fxaaFragShader;
        fxaaPipelineSpecs.layout = fxaaLayout;
        fxaaPipelineSpecs.colorFormats = { swapchain.getImageFormat() };
        fxaaPipelineSpecs.depthFormat = VK_FORMAT_UNDEFINED;
        fxaaPipelineSpecs.depthTest = false;
        fxaaPipelineSpecs.depthWrite = false;
        fxaaPipelineSpecs.cullMode = VK_CULL_MODE_NONE;

        auto fxaaPipeline = std::make_unique<astral::GraphicsPipeline>(&context, fxaaPipelineSpecs);
        // --------------------

        // --- Shadow Mapping Setup ---
        const uint32_t shadowMapSize = 4096;
        astral::ImageSpecs shadowSpecs;
        shadowSpecs.width = shadowMapSize;
        shadowSpecs.height = shadowMapSize;
        shadowSpecs.format = VK_FORMAT_D32_SFLOAT;
        shadowSpecs.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        shadowSpecs.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        shadowSpecs.arrayLayers = 4;
        shadowSpecs.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        auto shadowImage = std::make_unique<astral::Image>(&context, shadowSpecs);

        VkSampler shadowSampler;
        VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        vkCreateSampler(context.getDevice(), &samplerInfo, nullptr, &shadowSampler);
        
        uint32_t shadowMapIndex = context.getDescriptorManager().registerImageArray(shadowImage->getView(), shadowSampler);
        
        std::vector<VkImageView> shadowLayerViews(4);
        for (uint32_t i = 0; i < 4; i++) {
            VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            viewInfo.image = shadowImage->getHandle();
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_D32_SFLOAT;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = i;
            viewInfo.subresourceRange.layerCount = 1;
            vkCreateImageView(context.getDevice(), &viewInfo, nullptr, &shadowLayerViews[i]);
        }

        auto shadowVertShader = std::make_shared<astral::Shader>(&context, readFile("shaders/shadow.vert"), astral::ShaderStage::Vertex, "ShadowVert");
        auto shadowFragShader = std::make_shared<astral::Shader>(&context, readFile("shaders/shadow.frag"), astral::ShaderStage::Fragment, "ShadowFrag");

        astral::PipelineSpecs shadowPipelineSpecs;
        shadowPipelineSpecs.vertexShader = shadowVertShader;
        shadowPipelineSpecs.fragmentShader = shadowFragShader;
        shadowPipelineSpecs.layout = pipelineLayout;
        shadowPipelineSpecs.colorFormats = {}; // No color output for shadow pass
        shadowPipelineSpecs.depthFormat = VK_FORMAT_D32_SFLOAT;
        shadowPipelineSpecs.depthTest = true;
        shadowPipelineSpecs.depthWrite = true;
        shadowPipelineSpecs.cullMode = VK_CULL_MODE_FRONT_BIT;
        shadowPipelineSpecs.vertexBindings.push_back(vertexBinding);
        shadowPipelineSpecs.vertexAttributes = vertexAttrs;

        auto shadowPipeline = std::make_unique<astral::GraphicsPipeline>(&context, shadowPipelineSpecs);
        // -----------------------------

        // --- GPU Culling Setup ---
        auto cullShader = std::make_shared<astral::Shader>(&context, readFile("assets/shaders/cull.comp"), astral::ShaderStage::Compute, "CullShader");

        VkPushConstantRange cullPushRange = {};
        cullPushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        cullPushRange.offset = 0;
        cullPushRange.size = sizeof(uint32_t) * 4; // sceneDataIndex, instanceBufferIndex, indirectBufferIndex, instanceCount

        VkPipelineLayoutCreateInfo cullLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        cullLayoutInfo.pushConstantRangeCount = 1;
        cullLayoutInfo.pPushConstantRanges = &cullPushRange;
        cullLayoutInfo.setLayoutCount = 1;
        cullLayoutInfo.pSetLayouts = setLayouts;

        VkPipelineLayout cullLayout;
        vkCreatePipelineLayout(context.getDevice(), &cullLayoutInfo, nullptr, &cullLayout);

        astral::ComputePipelineSpecs cullSpecs;
        cullSpecs.computeShader = cullShader;
        cullSpecs.layout = cullLayout;
        auto cullPipeline = std::make_unique<astral::ComputePipeline>(&context, cullSpecs);
        // -------------------------

        // Skybox Shaders
        auto skyboxVertShader = std::make_shared<astral::Shader>(&context, readFile("assets/shaders/skybox.vert"), astral::ShaderStage::Vertex, "SkyboxVert");
        auto skyboxFragShader = std::make_shared<astral::Shader>(&context, readFile("assets/shaders/skybox.frag"), astral::ShaderStage::Fragment, "SkyboxFrag");

        // Skybox Pipeline Layout
        VkPushConstantRange skyboxPushRange = {};
        skyboxPushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        skyboxPushRange.offset = 0;
        skyboxPushRange.size = sizeof(uint32_t) * 2; // sceneDataIndex + skyboxTextureIndex

        VkPipelineLayoutCreateInfo skyboxLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        skyboxLayoutInfo.pushConstantRangeCount = 1;
        skyboxLayoutInfo.pPushConstantRanges = &skyboxPushRange;
        skyboxLayoutInfo.setLayoutCount = 1;
        skyboxLayoutInfo.pSetLayouts = setLayouts;

        VkPipelineLayout skyboxLayout;
        vkCreatePipelineLayout(context.getDevice(), &skyboxLayoutInfo, nullptr, &skyboxLayout);

        astral::PipelineSpecs skyboxSpecs;
        skyboxSpecs.vertexShader = skyboxVertShader;
        skyboxSpecs.fragmentShader = skyboxFragShader;
        skyboxSpecs.layout = skyboxLayout;
        skyboxSpecs.colorFormats = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT }; // Match MainPass (HDR + Normal)
        skyboxSpecs.depthFormat = VK_FORMAT_D32_SFLOAT;
        skyboxSpecs.depthTest = true;
        skyboxSpecs.depthWrite = false;
        skyboxSpecs.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Skybox is at depth 1.0
        skyboxSpecs.cullMode = VK_CULL_MODE_NONE;

        auto skyboxPipeline = std::make_unique<astral::GraphicsPipeline>(&context, skyboxSpecs);

        astral::UIManager uiManager(&context);

        // UI Parameters
        struct UIParams {
            float exposure = 1.0f;
            float bloomStrength = 0.04f;
            float bloomThreshold = 1.0f;
            float bloomSoftness = 0.5f;
            bool showSkybox = true;
            bool enableFXAA = true;
            bool enableHeadlamp = false;
            bool enableSSAO = true;
            bool visualizeCascades = false;
            float shadowBias = 0.002f;
            float shadowNormalBias = 0.005f;
            int pcfRange = 1;
            float csmLambda = 0.95f;
            float ssaoRadius = 0.5f;
            float ssaoBias = 0.025f;
            int selectedMaterial = 0;
            int selectedLight = 0;
        } uiParams;

        // Initialize default lights
        {
            astral::Light sun;
            sun.position = glm::vec4(5.0f, 8.0f, 5.0f, 1.0f); // Point light
            sun.color = glm::vec4(1.0f, 1.0f, 1.0f, 10.0f);   // White, 10 intensity
            sun.direction = glm::vec4(0.0f, -1.0f, 0.0f, 20.0f); // range 20
            sun.params = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);   // shadow index 0
            sceneManager.addLight(sun);

            astral::Light blueLight;
            blueLight.position = glm::vec4(-5.0f, 2.0f, -5.0f, 0.0f); // Point
            blueLight.color = glm::vec4(0.2f, 0.4f, 1.0f, 5.0f);
            blueLight.direction = glm::vec4(0.0f, 0.0f, 0.0f, 15.0f);
            sceneManager.addLight(blueLight);
        }

        astral::ImageSpecs ldrSpecs = hdrSpecs;
        ldrSpecs.format = VK_FORMAT_R8G8B8A8_UNORM;
        auto ldrImage = std::make_unique<astral::Image>(&context, ldrSpecs);
        uint32_t ldrTextureIndex = context.getDescriptorManager().registerImage(ldrImage->getView(), hdrSampler);

        uint32_t currentFrame = 0;
        float lastFrameTime = (float)glfwGetTime();

        spdlog::info("Initialization successful! Entering main loop...");

        while (!window.shouldClose()) {
            window.pollEvents();
            
            graph.clear(); // Clear passes each frame
            
            float currentTime = (float)glfwGetTime();
            float deltaTime = currentTime - lastFrameTime;
            lastFrameTime = currentTime;

            // Update Camera
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_W, true);
            else camera.processKeyboard(GLFW_KEY_W, false);
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_S, true);
            else camera.processKeyboard(GLFW_KEY_S, false);
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_A, true);
            else camera.processKeyboard(GLFW_KEY_A, false);
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_D, true);
            else camera.processKeyboard(GLFW_KEY_D, false);
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_Q) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_Q, true);
            else camera.processKeyboard(GLFW_KEY_Q, false);
            if (glfwGetKey(window.getNativeWindow(), GLFW_KEY_E) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_E, true);
            else camera.processKeyboard(GLFW_KEY_E, false);

            camera.update(deltaTime);

            uiManager.beginFrame();
            {
                ImGui::Begin("Renderer Controls");
                ImGui::DragFloat("Exposure", &uiParams.exposure, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Bloom Strength", &uiParams.bloomStrength, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("Bloom Threshold", &uiParams.bloomThreshold, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Bloom Softness", &uiParams.bloomSoftness, 0.01f, 0.0f, 1.0f);
                ImGui::Checkbox("Show Skybox", &uiParams.showSkybox);
                ImGui::Checkbox("Enable FXAA", &uiParams.enableFXAA);
                ImGui::Checkbox("Enable SSAO", &uiParams.enableSSAO);
                ImGui::Checkbox("Visualize CSM", &uiParams.visualizeCascades);
                
                if (ImGui::CollapsingHeader("Shadows & CSM")) {
                    ImGui::DragFloat("Shadow Bias", &uiParams.shadowBias, 0.0001f, 0.0f, 0.05f, "%.4f");
                    ImGui::DragFloat("Normal Bias", &uiParams.shadowNormalBias, 0.0001f, 0.0f, 0.05f, "%.4f");
                    ImGui::SliderInt("PCF Range", &uiParams.pcfRange, 0, 4);
                    ImGui::SliderFloat("CSM Lambda", &uiParams.csmLambda, 0.0f, 1.0f);
                }

                if (uiParams.enableSSAO) {
                    ImGui::Indent();
                    ImGui::DragFloat("SSAO Radius", &uiParams.ssaoRadius, 0.01f, 0.0f, 2.0f);
                    ImGui::DragFloat("SSAO Bias", &uiParams.ssaoBias, 0.001f, 0.0f, 0.1f);
                    ImGui::Unindent();
                }
                ImGui::Checkbox("Enable Headlamp", &uiParams.enableHeadlamp);
                ImGui::Separator();

                if (ImGui::CollapsingHeader("Light Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto& lights = sceneManager.getLights();
                    
                    if (ImGui::Button("Add Point Light")) {
                        astral::Light light;
                        light.position = glm::vec4(0, 5, 0, 0); // Point
                        light.color = glm::vec4(1, 1, 1, 1);
                        light.direction = glm::vec4(0, 0, 0, 10); // Range 10
                        sceneManager.addLight(light);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Add Sun Light")) {
                        astral::Light light;
                        light.position = glm::vec4(5, 8, 5, 1); // Directional
                        light.color = glm::vec4(1, 1, 0.9, 5);
                        light.direction = glm::vec4(normalize(glm::vec3(-1, -1, -1)), 0);
                        sceneManager.addLight(light);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clear All") && !lights.empty()) {
                        sceneManager.clearLights();
                        uiParams.selectedLight = 0;
                    }

                    if (!lights.empty()) {
                        if (uiParams.selectedLight >= (int)lights.size()) 
                            uiParams.selectedLight = (int)lights.size() - 1;

                        std::vector<std::string> lightNames;
                        for (size_t i = 0; i < lights.size(); ++i) {
                            std::string type = (lights[i].position.w == 1.0f) ? "Dir" : "Point";
                            lightNames.push_back(std::to_string(i) + ": " + type);
                        }

                        if (ImGui::BeginCombo("Select Light", lightNames[uiParams.selectedLight].c_str())) {
                            for (int i = 0; i < (int)lightNames.size(); i++) {
                                if (ImGui::Selectable(lightNames[i].c_str(), uiParams.selectedLight == i))
                                    uiParams.selectedLight = i;
                            }
                            ImGui::EndCombo();
                        }

                        auto light = lights[uiParams.selectedLight];
                        bool changed = false;

                        const char* types[] = { "Point", "Directional" };
                        int typeIdx = (int)light.position.w;
                        if (ImGui::Combo("Type", &typeIdx, types, 2)) {
                            light.position.w = (float)typeIdx;
                            changed = true;
                        }

                        changed |= ImGui::DragFloat3("Position", &light.position.x, 0.1f);
                        
                        glm::vec3 color = glm::vec3(light.color);
                        if (ImGui::ColorEdit3("Color", &color.x)) {
                            light.color.r = color.r;
                            light.color.g = color.g;
                            light.color.b = color.b;
                            changed = true;
                        }
                        changed |= ImGui::DragFloat("Intensity", &light.color.w, 0.1f, 0.0f, 100.0f);
                        
                        if (typeIdx == 1) { // Directional
                            glm::vec3 dir = glm::vec3(light.direction);
                            if (ImGui::DragFloat3("Direction", &dir.x, 0.01f, -1.0f, 1.0f)) {
                                light.direction = glm::vec4(normalize(dir), 0.0f);
                                changed = true;
                            }
                        } else {
                            changed |= ImGui::DragFloat("Range", &light.direction.w, 0.1f, 0.0f, 100.0f);
                        }

                        if (changed) {
                            sceneManager.updateLight(uiParams.selectedLight, light);
                        }

                        if (ImGui::Button("Remove Selected Light")) {
                            sceneManager.removeLight(uiParams.selectedLight);
                            if (uiParams.selectedLight > 0) uiParams.selectedLight--;
                        }
                    }
                }

                ImGui::Separator();

                if (ImGui::CollapsingHeader("Material Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto& materials = sceneManager.getMaterials();
                    if (!materials.empty()) {
                        std::vector<std::string> materialNames;
                        for (size_t i = 0; i < materials.size(); ++i) {
                            materialNames.push_back("Material " + std::to_string(i));
                        }

                        if (ImGui::BeginCombo("Select Material", materialNames[uiParams.selectedMaterial].c_str())) {
                            for (int i = 0; i < (int)materialNames.size(); i++) {
                                const bool is_selected = (uiParams.selectedMaterial == i);
                                if (ImGui::Selectable(materialNames[i].c_str(), is_selected))
                                    uiParams.selectedMaterial = i;
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }

                        auto mat = sceneManager.getMaterial(uiParams.selectedMaterial);
                        bool changed = false;

                        changed |= ImGui::ColorEdit4("Base Color", &mat.baseColorFactor.x);
                        changed |= ImGui::SliderFloat("Metallic", &mat.metallicFactor, 0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Roughness", &mat.roughnessFactor, 0.0f, 1.0f);
                        changed |= ImGui::SliderFloat("Alpha Cutoff", &mat.alphaCutoff, 0.0f, 1.0f);

                        if (changed) {
                            sceneManager.updateMaterial(uiParams.selectedMaterial, mat);
                        }
                    } else {
                        ImGui::Text("No materials loaded.");
                    }
                }

                ImGui::Separator();
                ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
                ImGui::End();
            }
            uiManager.endFrame();

            static float logTimer = 0.0f;
            logTimer += deltaTime;
            if (logTimer > 1.0f) {
                auto pos = camera.getPosition();
                spdlog::info("Camera Pos: ({:.2f}, {:.2f}, {:.2f}) | dt: {:.4f}", pos.x, pos.y, pos.z, deltaTime);
                logTimer = 0.0f;
            }

            // Update Scene Data
            astral::SceneData sd;
            sd.view = camera.getViewMatrix();
            sd.proj = camera.getProjectionMatrix();
            sd.viewProj = sd.proj * sd.view;
            sd.invView = glm::inverse(sd.view);
            sd.invProj = glm::inverse(sd.proj);
            sd.cameraPos = glm::vec4(camera.getPosition(), 1.0f);

            // Frustum Planes Calculation
            glm::mat4 vp = sd.viewProj;
            for (int i = 0; i < 4; i++) sd.frustumPlanes[0][i] = vp[i][3] + vp[i][0]; // Left
            for (int i = 0; i < 4; i++) sd.frustumPlanes[1][i] = vp[i][3] - vp[i][0]; // Right
            for (int i = 0; i < 4; i++) sd.frustumPlanes[2][i] = vp[i][3] + vp[i][1]; // Bottom
            for (int i = 0; i < 4; i++) sd.frustumPlanes[3][i] = vp[i][3] - vp[i][1]; // Top
            for (int i = 0; i < 4; i++) sd.frustumPlanes[4][i] = vp[i][3] + vp[i][2]; // Near
            for (int i = 0; i < 4; i++) sd.frustumPlanes[5][i] = vp[i][3] - vp[i][2]; // Far

            // Normalize planes
            for (int i = 0; i < 6; i++) {
                float length = glm::length(glm::vec3(sd.frustumPlanes[i]));
                sd.frustumPlanes[i] /= length;
            }
            
            // Light Space Matrix for Shadow Mapping (using the first light)
            auto& lights = sceneManager.getLights();
            glm::vec3 lightPos = glm::vec3(5.0f, 8.0f, 5.0f);
            glm::vec3 lightDir = normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
            bool isDirectional = true;

            if (!lights.empty()) {
                isDirectional = (lights[0].position.w == 1.0f);
                if (isDirectional) {
                    lightDir = normalize(glm::vec3(lights[0].direction));
                    lightPos = -lightDir * 10.0f; // Position "far away" for ortho projection
                } else {
                    lightPos = glm::vec3(lights[0].position);
                    lightDir = normalize(glm::vec3(0.0f) - lightPos);
                }
            }

            glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
            float orthoSize = 10.0f; 
            glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f); 
            lightProjection[1][1] *= -1; // Vulkan Y flip
            sd.lightSpaceMatrix = lightProjection * lightView;

            // Cascaded Shadow Maps (CSM) Calculation
            float nearClip = camera.getNear();
            float farClip = camera.getFar();
            float cascadeSplits[4];

            float lambda = uiParams.csmLambda;
            float ratio = farClip / nearClip;

            for (int i = 0; i < 4; i++) {
                float p = (i + 1) / 4.0f;
                float log = nearClip * std::pow(ratio, p);
                float uniform = nearClip + (farClip - nearClip) * p;
                float d = lambda * (log - uniform) + uniform;
                cascadeSplits[i] = d;
            }

            sd.cascadeSplits = glm::vec4(cascadeSplits[0], cascadeSplits[1], cascadeSplits[2], cascadeSplits[3]);

            float lastSplitDist = nearClip;
            for (int i = 0; i < 4; i++) {
                float splitDist = cascadeSplits[i];

                glm::vec3 frustumCorners[8] = {
                    {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f},
                    {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f}
                };

                glm::mat4 invCam = glm::inverse(camera.getProjectionMatrix() * camera.getViewMatrix());
                for (int j = 0; j < 8; j++) {
                    glm::vec4 pt = invCam * glm::vec4(frustumCorners[j], 1.0f);
                    frustumCorners[j] = glm::vec3(pt) / pt.w;
                }

                for (int j = 0; j < 4; j++) {
                    glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
                    frustumCorners[j + 4] = frustumCorners[j] + (dist * (splitDist / farClip));
                    frustumCorners[j] = frustumCorners[j] + (dist * (lastSplitDist / farClip));
                }

                glm::vec3 center = glm::vec3(0.0f);
                for (int j = 0; j < 8; j++) center += frustumCorners[j];
                center /= 8.0f;

                float radius = 0.0f;
                for (int j = 0; j < 8; j++) radius = std::max(radius, glm::length(frustumCorners[j] - center));
                radius = std::ceil(radius * 16.0f) / 16.0f;

                glm::vec3 maxExtents = glm::vec3(radius);
                glm::vec3 minExtents = -maxExtents;

                glm::mat4 lightViewMatrix = glm::lookAt(center - lightDir * radius, center, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, 2.0f * radius);

                // Stable CSM: Snap to texel size
                glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
                glm::vec4 shadowOrigin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                shadowOrigin = shadowMatrix * shadowOrigin;
                shadowOrigin *= (float)shadowMapSize / 2.0f;

                glm::vec2 roundedOrigin = glm::round(glm::vec2(shadowOrigin));
                glm::vec2 roundOffset = (roundedOrigin - glm::vec2(shadowOrigin)) * 2.0f / (float)shadowMapSize;

                lightOrthoMatrix[3][0] += roundOffset.x;
                lightOrthoMatrix[3][1] += roundOffset.y;

                lightOrthoMatrix[1][1] *= -1;

                sd.cascadeViewProj[i] = lightOrthoMatrix * lightViewMatrix;
                lastSplitDist = splitDist;
            }

            sd.lightCount = (int)lights.size();
            sd.lightBufferIndex = sceneManager.getLightBufferIndex();
            sd.headlampEnabled = uiParams.enableHeadlamp ? 1 : 0;
            sd.visualizeCascades = uiParams.visualizeCascades ? 1 : 0;
            sd.shadowBias = uiParams.shadowBias;
            sd.shadowNormalBias = uiParams.shadowNormalBias;
            sd.pcfRange = uiParams.pcfRange;
            sd.csmLambda = uiParams.csmLambda;
            sd.irradianceIndex = envManager.getIrradianceIndex();
            sd.prefilteredIndex = envManager.getPrefilteredIndex();
            sd.brdfLutIndex = envManager.getBrdfLutIndex();
            sd.shadowMapIndex = (int)shadowMapIndex;
            sceneManager.updateSceneData(sd);
            
            // Clear and re-add mesh instances for this frame
            sceneManager.clearMeshInstances();
            if (model) {
                for (const auto& mesh : model->meshes) {
                    for (const auto& primitive : mesh.primitives) {
                        sceneManager.addMeshInstance(
                            glm::mat4(1.0f), 
                            primitive.materialIndex, 
                            primitive.indexCount, 
                            primitive.firstIndex, 
                            0, // vertexOffset
                            primitive.boundingCenter,
                            primitive.boundingRadius
                        );
                    }
                }
            }

            sync.waitForFrame(currentFrame);
            
            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(context.getDevice(), swapchain.getHandle(), UINT64_MAX, sync.getImageAvailableSemaphore(currentFrame), VK_NULL_HANDLE, &imageIndex);
            
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                // Handle resize
                continue;
            }

            sync.resetFence(currentFrame);

            cmd->begin();
            
            graph.addExternalResource("Swapchain", swapchain.getImages()[imageIndex], swapchain.getImageViews()[imageIndex], swapchain.getImageFormat(), swapchain.getExtent().width, swapchain.getExtent().height, VK_IMAGE_LAYOUT_UNDEFINED);
            VkClearValue colorClear;
            colorClear.color = {{0.1f, 0.2f, 0.4f, 1.0f}}; // Bright blue-ish
            graph.setResourceClearValue("Swapchain", colorClear);

            graph.addExternalResource("HDR_Color", hdrImage->getHandle(), hdrImage->getView(), hdrImage->getSpecs().format, swapchain.getExtent().width, swapchain.getExtent().height, VK_IMAGE_LAYOUT_UNDEFINED);
            graph.setResourceClearValue("HDR_Color", colorClear);

            graph.addExternalResource("Normal", normalImage->getHandle(), normalImage->getView(), normalImage->getSpecs().format, swapchain.getExtent().width, swapchain.getExtent().height, VK_IMAGE_LAYOUT_UNDEFINED);
            graph.setResourceClearValue("Normal", colorClear);

            graph.addExternalResource("Depth", depthImage->getHandle(), depthImage->getView(), depthImage->getSpecs().format, swapchain.getExtent().width, swapchain.getExtent().height, VK_IMAGE_LAYOUT_UNDEFINED);
            
            VkClearValue depthClear;
            depthClear.depthStencil = {1.0f, 0};
            graph.setResourceClearValue("Depth", depthClear);

            VkClearValue shadowClear;
            shadowClear.depthStencil = {1.0f, 0};

            graph.addExternalResource("ShadowMap", shadowImage->getHandle(), shadowImage->getView(), shadowSpecs.format, shadowMapSize, shadowMapSize, VK_IMAGE_LAYOUT_UNDEFINED);

            graph.addPass("CullingPass", {}, {}, [&](VkCommandBuffer cb) {
                vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, cullPipeline->getHandle());
                
                VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, cullLayout, 0, 1, &globalSet, 0, nullptr);

                struct CullPushConstants {
                    uint32_t sceneDataIndex;
                    uint32_t instanceBufferIndex;
                    uint32_t indirectBufferIndex;
                    uint32_t instanceCount;
                } cpc;

                cpc.sceneDataIndex = sceneManager.getSceneBufferIndex();
                cpc.instanceBufferIndex = sceneManager.getMeshInstanceBufferIndex();
                cpc.indirectBufferIndex = sceneManager.getIndirectBufferIndex();
                cpc.instanceCount = static_cast<uint32_t>(sceneManager.getMeshInstanceCount());

                vkCmdPushConstants(cb, cullLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CullPushConstants), &cpc);

                uint32_t groupCount = (cpc.instanceCount + 63) / 64;
                if (groupCount > 0) {
                    vkCmdDispatch(cb, groupCount, 1, 1);
                }

                // Memory barrier to ensure indirect commands are ready for the graphics pipeline
                VkBufferMemoryBarrier barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
                barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                barrier.buffer = sceneManager.getIndirectBuffer();
                barrier.offset = 0;
                barrier.size = VK_WHOLE_SIZE;

                vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
            });

            for (uint32_t i = 0; i < 4; i++) {
                std::string resName = "ShadowMap_" + std::to_string(i);
                graph.addExternalResource(resName, shadowImage->getHandle(), shadowLayerViews[i], shadowSpecs.format, shadowMapSize, shadowMapSize, VK_IMAGE_LAYOUT_UNDEFINED);
                graph.setResourceClearValue(resName, shadowClear);

                graph.addPass("ShadowPass_" + std::to_string(i), {}, {resName}, [&](VkCommandBuffer cb) {
                    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline->getHandle());
                    
                    VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalSet, 0, nullptr);

                    VkExtent2D shadowExtent = {shadowMapSize, shadowMapSize};
                    VkViewport viewport = {0.0f, 0.0f, static_cast<float>(shadowExtent.width), static_cast<float>(shadowExtent.height), 0.0f, 1.0f};
                    vkCmdSetViewport(cb, 0, 1, &viewport);
                    VkRect2D scissor = {{0, 0}, shadowExtent};
                    vkCmdSetScissor(cb, 0, 1, &scissor);

                    if (model) {
                        VkDeviceSize offsets[] = {0};
                        VkBuffer vBuffer = model->vertexBuffer->getHandle();
                        vkCmdBindVertexBuffers(cb, 0, 1, &vBuffer, offsets);
                        vkCmdBindIndexBuffer(cb, model->indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);

                        struct PushConstants {
                            uint32_t sceneDataIndex;
                            uint32_t instanceBufferIndex;
                            uint32_t materialBufferIndex;
                            uint32_t cascadeIndex; 
                        } spc;
                        
                        spc.sceneDataIndex = sceneManager.getSceneBufferIndex();
                        spc.instanceBufferIndex = sceneManager.getMeshInstanceBufferIndex();
                        spc.materialBufferIndex = sceneManager.getMaterialBufferIndex();
                        spc.cascadeIndex = i;
                        
                        vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &spc);
                        
                        vkCmdDrawIndexedIndirect(
                            cb, 
                            sceneManager.getIndirectBuffer(), 
                            0, 
                            static_cast<uint32_t>(sceneManager.getMeshInstanceCount()), 
                            sizeof(VkDrawIndexedIndirectCommand)
                        );
                    }
                });
            }

            graph.addExternalResource("Bloom_Base", bloomImage->getHandle(), bloomImage->getView(), bloomImage->getSpecs().format, bloomSpecs.width, bloomSpecs.height, VK_IMAGE_LAYOUT_UNDEFINED);
            graph.addExternalResource("Bloom_Blur", bloomBlurImage->getHandle(), bloomBlurImage->getView(), bloomBlurImage->getSpecs().format, bloomSpecs.width, bloomSpecs.height, VK_IMAGE_LAYOUT_UNDEFINED);

            graph.addExternalResource("SSAO_Base", ssaoImage->getHandle(), ssaoImage->getView(), ssaoImage->getSpecs().format, swapchain.getExtent().width, swapchain.getExtent().height, VK_IMAGE_LAYOUT_UNDEFINED);
            graph.addExternalResource("SSAO_Blur", ssaoBlurImage->getHandle(), ssaoBlurImage->getView(), ssaoBlurImage->getSpecs().format, swapchain.getExtent().width, swapchain.getExtent().height, VK_IMAGE_LAYOUT_UNDEFINED);
            
            VkClearValue ssaoClear;
            ssaoClear.color = {{1.0f, 0.0f, 0.0f, 0.0f}};
            graph.setResourceClearValue("SSAO_Base", ssaoClear);
            graph.setResourceClearValue("SSAO_Blur", ssaoClear);

            graph.addExternalResource("LDR_Color", ldrImage->getHandle(), ldrImage->getView(), ldrImage->getSpecs().format, swapchain.getExtent().width, swapchain.getExtent().height, VK_IMAGE_LAYOUT_UNDEFINED);

            graph.addPass("MainPass", {"ShadowMap", "ShadowMap_0", "ShadowMap_1", "ShadowMap_2", "ShadowMap_3"}, {"HDR_Color", "Normal", "Depth"}, [&](VkCommandBuffer cb) {
                // 1. Draw Skybox
                if (uiParams.showSkybox) {
                    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline->getHandle());
                    
                    VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxLayout, 0, 1, &globalSet, 0, nullptr);

                    struct SkyboxPushConstants {
                        uint32_t sceneDataIndex;
                        uint32_t skyboxTextureIndex;
                    } skyboxPc;
                    skyboxPc.sceneDataIndex = sceneManager.getSceneBufferIndex();
                    skyboxPc.skyboxTextureIndex = envManager.getSkyboxIndex();
                    vkCmdPushConstants(cb, skyboxLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyboxPushConstants), &skyboxPc);

                    VkExtent2D extent = swapchain.getExtent();
                    VkViewport viewport = {0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
                    vkCmdSetViewport(cb, 0, 1, &viewport);
                    VkRect2D scissor = {{0, 0}, extent};
                    vkCmdSetScissor(cb, 0, 1, &scissor);

                    vkCmdDraw(cb, 36, 1, 0, 0);
                }

                // 2. Draw Model
                vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getHandle());
                
                if (model) {
                    VkDeviceSize offsets[] = {0};
                    VkBuffer vBuffer = model->vertexBuffer->getHandle();
                    vkCmdBindVertexBuffers(cb, 0, 1, &vBuffer, offsets);
                    vkCmdBindIndexBuffer(cb, model->indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);

                    struct PushConstants {
                        uint32_t sceneDataIndex;
                        uint32_t instanceBufferIndex;
                        uint32_t materialBufferIndex;
                        uint32_t padding;
                    } pc;
                    
                    pc.sceneDataIndex = sceneManager.getSceneBufferIndex();
                    pc.instanceBufferIndex = sceneManager.getMeshInstanceBufferIndex();
                    pc.materialBufferIndex = sceneManager.getMaterialBufferIndex();
                    pc.padding = 0;
    
                    VkDescriptorSet currentGlobalSet = context.getDescriptorManager().getDescriptorSet();
                    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &currentGlobalSet, 0, nullptr);
                    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);
                    
                    vkCmdDrawIndexedIndirect(
                        cb, 
                        sceneManager.getIndirectBuffer(), 
                        0, 
                        static_cast<uint32_t>(sceneManager.getMeshInstanceCount()), 
                        sizeof(VkDrawIndexedIndirectCommand)
                    );
                }
            });

            graph.addPass("SSAOPass", {"Normal", "Depth"}, {"SSAO_Base"}, [&](VkCommandBuffer cb) {
                if (uiParams.enableSSAO) {
                    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoPipeline->getHandle());
                    VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoLayout, 0, 1, &globalSet, 0, nullptr);

                    struct SSAOPushConstants {
                        int normalTextureIndex;
                        int depthTextureIndex;
                        int noiseTextureIndex;
                        int kernelBufferIndex;
                        float radius;
                        float bias;
                    } spc;
                    spc.normalTextureIndex = (int)normalTextureIndex;
                    spc.depthTextureIndex = (int)depthTextureIndex;
                    spc.noiseTextureIndex = (int)noiseTextureIndex;
                    spc.kernelBufferIndex = (int)ssaoKernelBufferIndex;
                    spc.radius = uiParams.ssaoRadius;
                    spc.bias = uiParams.ssaoBias;
                    vkCmdPushConstants(cb, ssaoLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SSAOPushConstants), &spc);

                    VkExtent2D extent = swapchain.getExtent();
                    VkViewport viewport = {0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
                    vkCmdSetViewport(cb, 0, 1, &viewport);
                    VkRect2D scissor = {{0, 0}, extent};
                    vkCmdSetScissor(cb, 0, 1, &scissor);

                    vkCmdDraw(cb, 3, 1, 0, 0);
                }
            });

            graph.addPass("SSAOBlurPass", {"SSAO_Base"}, {"SSAO_Blur"}, [&](VkCommandBuffer cb) {
                if (uiParams.enableSSAO) {
                    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoBlurPipeline->getHandle());
                    VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ssaoBlurLayout, 0, 1, &globalSet, 0, nullptr);

                    struct SSAOBlurPushConstants {
                        int inputTextureIndex;
                    } sbpc;
                    sbpc.inputTextureIndex = (int)ssaoTextureIndex;
                    vkCmdPushConstants(cb, ssaoBlurLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SSAOBlurPushConstants), &sbpc);

                    VkExtent2D extent = swapchain.getExtent();
                    VkViewport viewport = {0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
                    vkCmdSetViewport(cb, 0, 1, &viewport);
                    VkRect2D scissor = {{0, 0}, extent};
                    vkCmdSetScissor(cb, 0, 1, &scissor);

                    vkCmdDraw(cb, 3, 1, 0, 0);
                }
            });

            graph.addPass("BloomExtract", {"HDR_Color"}, {"Bloom_Base"}, [&](VkCommandBuffer cb) {
                vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomPipeline->getHandle());
                VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomLayout, 0, 1, &globalSet, 0, nullptr);

                struct BloomPushConstants {
                    int inputTextureIndex;
                    int mode;
                    float threshold;
                    float softness;
                } bpc;
                bpc.inputTextureIndex = (int)hdrTextureIndex;
                bpc.mode = 0; // Extraction
                bpc.threshold = uiParams.bloomThreshold;
                bpc.softness = uiParams.bloomSoftness;
                vkCmdPushConstants(cb, bloomLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomPushConstants), &bpc);

                VkViewport viewport = {0.0f, 0.0f, static_cast<float>(bloomSpecs.width), static_cast<float>(bloomSpecs.height), 0.0f, 1.0f};
                vkCmdSetViewport(cb, 0, 1, &viewport);
                VkRect2D scissor = {{0, 0}, {bloomSpecs.width, bloomSpecs.height}};
                vkCmdSetScissor(cb, 0, 1, &scissor);

                vkCmdDraw(cb, 3, 1, 0, 0);
            });

            graph.addPass("BloomBlurH", {"Bloom_Base"}, {"Bloom_Blur"}, [&](VkCommandBuffer cb) {
                vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomPipeline->getHandle());
                VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomLayout, 0, 1, &globalSet, 0, nullptr);

                struct BloomPushConstants {
                    int inputTextureIndex;
                    int mode;
                    float threshold;
                    float softness;
                } bpc;
                bpc.inputTextureIndex = (int)bloomTextureIndex;
                bpc.mode = 1; // H Blur
                bpc.threshold = uiParams.bloomThreshold;
                bpc.softness = uiParams.bloomSoftness;
                vkCmdPushConstants(cb, bloomLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomPushConstants), &bpc);

                VkViewport viewport = {0.0f, 0.0f, static_cast<float>(bloomSpecs.width), static_cast<float>(bloomSpecs.height), 0.0f, 1.0f};
                vkCmdSetViewport(cb, 0, 1, &viewport);
                VkRect2D scissor = {{0, 0}, {bloomSpecs.width, bloomSpecs.height}};
                vkCmdSetScissor(cb, 0, 1, &scissor);

                vkCmdDraw(cb, 3, 1, 0, 0);
            });

            graph.addPass("BloomBlurV", {"Bloom_Blur"}, {"Bloom_Base"}, [&](VkCommandBuffer cb) {
                vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomPipeline->getHandle());
                VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomLayout, 0, 1, &globalSet, 0, nullptr);

                struct BloomPushConstants {
                    int inputTextureIndex;
                    int mode;
                    float threshold;
                    float softness;
                } bpc;
                bpc.inputTextureIndex = (int)bloomBlurTextureIndex;
                bpc.mode = 2; // V Blur
                bpc.threshold = uiParams.bloomThreshold;
                bpc.softness = uiParams.bloomSoftness;
                vkCmdPushConstants(cb, bloomLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomPushConstants), &bpc);

                VkViewport viewport = {0.0f, 0.0f, static_cast<float>(bloomSpecs.width), static_cast<float>(bloomSpecs.height), 0.0f, 1.0f};
                vkCmdSetViewport(cb, 0, 1, &viewport);
                VkRect2D scissor = {{0, 0}, {bloomSpecs.width, bloomSpecs.height}};
                vkCmdSetScissor(cb, 0, 1, &scissor);

                vkCmdDraw(cb, 3, 1, 0, 0);
            });

            graph.addPass("Composite", {"HDR_Color", "Bloom_Base", "SSAO_Blur"}, {"LDR_Color"}, [&](VkCommandBuffer cb) {
                vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline->getHandle());
                
                VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, compositeLayout, 0, 1, &globalSet, 0, nullptr);

                struct CompositePushConstants {
                    int hdrTextureIndex;
                    int bloomTextureIndex;
                    int ssaoTextureIndex;
                    float exposure;
                    float bloomStrength;
                    int enableSSAO;
                } cpc;
                cpc.hdrTextureIndex = (int)hdrTextureIndex;
                cpc.bloomTextureIndex = (int)bloomTextureIndex; // Use Bloom_Base (final result after V-Blur)
                cpc.ssaoTextureIndex = (int)ssaoBlurTextureIndex;
                cpc.exposure = uiParams.exposure;
                cpc.bloomStrength = uiParams.bloomStrength;
                cpc.enableSSAO = uiParams.enableSSAO ? 1 : 0;
                
                vkCmdPushConstants(cb, compositeLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants), &cpc);
                
                VkExtent2D extent = swapchain.getExtent();
                VkViewport viewport = {0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
                vkCmdSetViewport(cb, 0, 1, &viewport);
                VkRect2D scissor = {{0, 0}, extent};
                vkCmdSetScissor(cb, 0, 1, &scissor);

                vkCmdDraw(cb, 3, 1, 0, 0);
            });

            graph.addPass("FXAA", {"LDR_Color"}, {"Swapchain"}, [&](VkCommandBuffer cb) {
                vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, fxaaPipeline->getHandle());
                
                VkDescriptorSet globalSet = context.getDescriptorManager().getDescriptorSet();
                vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, fxaaLayout, 0, 1, &globalSet, 0, nullptr);

                struct FXAAPushConstants {
                    int inputTextureIndex;
                    int padding;
                    glm::vec2 inverseScreenSize;
                } fpc;
                fpc.inputTextureIndex = (int)ldrTextureIndex;
                fpc.padding = 0;

                if (uiParams.enableFXAA) {
                    fpc.inverseScreenSize = 1.0f / glm::vec2(swapchain.getExtent().width, swapchain.getExtent().height);
                } else {
                    fpc.inverseScreenSize = glm::vec2(0.0f); // 0 span = no blur
                }
                
                vkCmdPushConstants(cb, fxaaLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FXAAPushConstants), &fpc);
                
                VkExtent2D extent = swapchain.getExtent();
                VkViewport viewport = {0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
                vkCmdSetViewport(cb, 0, 1, &viewport);
                VkRect2D scissor = {{0, 0}, extent};
                vkCmdSetScissor(cb, 0, 1, &scissor);

                vkCmdDraw(cb, 3, 1, 0, 0);
            });

            graph.addPass("UIPass", {}, {"Swapchain"}, [&](VkCommandBuffer cb) {
                uiManager.render(cb);
            }, false); // false = don't clear, load previous content (composite result)

            graph.execute(cmd->getHandle(), swapchain.getExtent());

            cmd->end();

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkSemaphore waitSemaphores[] = {sync.getImageAvailableSemaphore(currentFrame)};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            auto cmdHandle = cmd->getHandle();
            submitInfo.pCommandBuffers = &cmdHandle;
            VkSemaphore signalSemaphores[] = {sync.getRenderFinishedSemaphore(currentFrame)};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            vkQueueSubmit(context.getGraphicsQueue(), 1, &submitInfo, sync.getInFlightFence(currentFrame));
            
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            VkSwapchainKHR swapchains[] = {swapchain.getHandle()};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &imageIndex;
            
            result = vkQueuePresentKHR(context.getPresentQueue(), &presentInfo);
            
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                // Handle resize
            }

            currentFrame = (currentFrame + 1) % 1;
        }

        vkDeviceWaitIdle(context.getDevice());
        
        vkDestroyPipelineLayout(context.getDevice(), pipelineLayout, nullptr);

    } catch (const std::exception& e) {
        spdlog::error("Critical error: {}", e.what());
        return 1;
    }

    return EXIT_SUCCESS;
}
