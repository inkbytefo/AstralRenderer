#include "astral/renderer/scene_manager.hpp"
#include "astral/renderer/descriptor_manager.hpp"
#include <stdexcept>

namespace astral {

SceneManager::SceneManager(Context* context) : m_context(context) {
    // Scene Data Buffer (Uniform-like but as Storage for bindless flexibility)
    m_sceneBuffer = std::make_unique<Buffer>(
        m_context, 
        sizeof(SceneData), 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Material Metadata Buffer
    m_materialBuffer = std::make_unique<Buffer>(
        m_context, 
        sizeof(MaterialMetadata) * MAX_MATERIALS, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Light Buffer
    m_lightBuffer = std::make_unique<Buffer>(
        m_context,
        sizeof(Light) * MAX_LIGHTS,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Register with Bindless Manager
    auto& descriptorManager = m_context->getDescriptorManager();
    m_sceneBufferIndex = descriptorManager.registerBuffer(m_sceneBuffer->getHandle(), 0, sizeof(SceneData), 1);
    m_materialBufferIndex = descriptorManager.registerBuffer(m_materialBuffer->getHandle(), 0, sizeof(MaterialMetadata) * MAX_MATERIALS, 2);
    m_lightBufferIndex = descriptorManager.registerBuffer(m_lightBuffer->getHandle(), 0, sizeof(Light) * MAX_LIGHTS, 3);
    
    m_materials.reserve(MAX_MATERIALS);
    m_lights.reserve(MAX_LIGHTS);
}

void SceneManager::updateSceneData(const SceneData& data) {
    m_sceneBuffer->upload(&data, sizeof(SceneData));
}

uint32_t SceneManager::addLight(const Light& light) {
    if (m_lights.size() >= MAX_LIGHTS) {
        throw std::runtime_error("Maximum lights reached in SceneManager!");
    }
    uint32_t index = static_cast<uint32_t>(m_lights.size());
    m_lights.push_back(light);
    updateLight(index, light);
    return index;
}

void SceneManager::updateLight(uint32_t index, const Light& light) {
    if (index >= m_lights.size()) return;
    m_lights[index] = light;
    m_lightBuffer->upload(&light, sizeof(Light), sizeof(Light) * index);
}

void SceneManager::removeLight(uint32_t index) {
    if (index >= m_lights.size()) return;
    m_lights.erase(m_lights.begin() + index);
    
    // Re-upload the whole light array to the buffer
    if (!m_lights.empty()) {
        m_lightBuffer->upload(m_lights.data(), sizeof(Light) * m_lights.size(), 0);
    }
}

void SceneManager::clearLights() {
    m_lights.clear();
}

uint32_t SceneManager::addMaterial(const MaterialMetadata& material) {
    if (m_materials.size() >= MAX_MATERIALS) {
        throw std::runtime_error("Maximum materials reached in SceneManager!");
    }
    
    uint32_t index = static_cast<uint32_t>(m_materials.size());
    m_materials.push_back(material);
    
    updateMaterial(index, material);
    return index;
}

void SceneManager::updateMaterial(uint32_t index, const MaterialMetadata& material) {
    if (index >= m_materials.size()) return;
    
    m_materials[index] = material;
    m_materialBuffer->upload(&material, sizeof(MaterialMetadata), sizeof(MaterialMetadata) * index);
}

} // namespace astral
