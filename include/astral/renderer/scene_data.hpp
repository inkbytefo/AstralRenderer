#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace astral {

struct Light {
    glm::vec4 position;  // w = type (0: Point, 1: Directional, 2: Spot)
    glm::vec4 color;     // w = intensity
    glm::vec4 direction; // w = range (for point/spot)
    glm::vec4 params;    // x = innerCutoff, y = outerCutoff, z = shadowIndex, w = padding
};

struct SceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::mat4 invView;
    glm::mat4 invProj;
    glm::mat4 lightSpaceMatrix; // Primary shadow matrix
    glm::mat4 cascadeViewProj[4];
    glm::vec4 cascadeSplits;
    glm::vec4 cameraPos;
    int lightCount;
    int irradianceIndex;
    int prefilteredIndex;
    int brdfLutIndex;
    int shadowMapIndex;
    int lightBufferIndex; // Index to the SSBO containing Light array
    int headlampEnabled;
    int visualizeCascades;
    float shadowBias;
    float shadowNormalBias;
    int pcfRange;
    float csmLambda;
    int padding1;
};

struct MaterialMetadata {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    int baseColorTextureIndex;
    int metallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;
    float alphaCutoff;
    float padding1;
    float padding2;
    float padding3;
    float padding4;
};

} // namespace astral
