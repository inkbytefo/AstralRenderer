#pragma once

#include "astral/core/context.hpp"
#include "astral/resources/buffer.hpp"
#include "astral/renderer/scene_data.hpp"
#include <memory>
#include <vector>

namespace astral {

class SceneManager {
public:
    SceneManager(Context* context);
    ~SceneManager() = default;

    void updateSceneData(const SceneData& data);
    
    // Light management
    uint32_t addLight(const Light& light);
    void updateLight(uint32_t index, const Light& light);
    void removeLight(uint32_t index);
    void clearLights();
    const std::vector<Light>& getLights() const { return m_lights; }
    uint32_t getLightBufferIndex() const { return m_lightBufferIndex; }

    // Material management
    uint32_t addMaterial(const MaterialMetadata& material);
    void updateMaterial(uint32_t index, const MaterialMetadata& material);

    const std::vector<MaterialMetadata>& getMaterials() const { return m_materials; }
    size_t getMaterialCount() const { return m_materials.size(); }
    const MaterialMetadata& getMaterial(uint32_t index) const { return m_materials[index]; }

    VkBuffer getSceneBuffer() const { return m_sceneBuffer->getHandle(); }
    VkBuffer getMaterialBuffer() const { return m_materialBuffer->getHandle(); }
    
    uint32_t getSceneBufferIndex() const { return m_sceneBufferIndex; }
    uint32_t getMaterialBufferIndex() const { return m_materialBufferIndex; }
    uint32_t getMeshInstanceBufferIndex() const { return m_meshInstanceBufferIndex; }
    uint32_t getIndirectBufferIndex() const { return m_indirectBufferIndex; }

    void addMeshInstance(const glm::mat4& transform, uint32_t materialIndex, uint32_t indexCount, uint32_t firstIndex, int vertexOffset, const glm::vec3& center, float radius);
    void prepareIndirectCommands();
    void clearMeshInstances();
    
    size_t getMeshInstanceCount() const { return m_meshInstances.size(); }

    VkBuffer getMeshInstanceBuffer() const { return m_meshInstanceBuffer->getHandle(); }
    VkBuffer getIndirectBuffer() const { return m_indirectBuffer->getHandle(); }

private:
    Context* m_context;
    
    struct MeshInstance {
        glm::mat4 transform;
        glm::vec3 sphereCenter;
        float sphereRadius;
        uint32_t materialIndex;
        uint32_t padding[3];
    };

    std::unique_ptr<Buffer> m_sceneBuffer;
    std::unique_ptr<Buffer> m_materialBuffer;
    std::unique_ptr<Buffer> m_lightBuffer;
    std::unique_ptr<Buffer> m_meshInstanceBuffer;
    std::unique_ptr<Buffer> m_indirectBuffer;
    
    uint32_t m_sceneBufferIndex;
    uint32_t m_materialBufferIndex;
    uint32_t m_lightBufferIndex;
    uint32_t m_meshInstanceBufferIndex;
    uint32_t m_indirectBufferIndex;
    
    std::vector<MaterialMetadata> m_materials;
    std::vector<Light> m_lights;
    std::vector<MeshInstance> m_meshInstances;

    static constexpr uint32_t MAX_MATERIALS = 1000;
    static constexpr uint32_t MAX_LIGHTS = 256;
    static constexpr uint32_t MAX_MESH_INSTANCES = 10000;
};

} // namespace astral
