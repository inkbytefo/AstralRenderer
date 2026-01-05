#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inPos;
// layout(location = 1) in vec3 inNormal;
// layout(location = 2) in vec2 inUV;
// layout(location = 3) in vec4 inTangent;
// layout(location = 4) in vec4 inColor;

struct SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invView;
    mat4 invProj;
    mat4 lightSpaceMatrix;
    mat4 cascadeViewProj[4];
    vec4 cascadeSplits;
    vec4 cameraPos;
    int lightCount;
    int irradianceIndex;
    int prefilteredIndex;
    int brdfLutIndex;
    int shadowMapIndex;
    int lightBufferIndex;
    int headlampEnabled;
    int visualizeCascades;
    float shadowBias;
    float shadowNormalBias;
    int pcfRange;
    float csmLambda;
    int padding1;
};

// Bindless Set #0
layout(std430, set = 0, binding = 1) readonly buffer SceneDataBuffer {
    SceneData scene;
} allSceneBuffers[];

// Push Constants
layout(push_constant) uniform PushConstants {
    mat4 model;
    uint sceneDataIndex;
    uint materialIndex;
    uint materialBufferIndex;
    uint cascadeIndex; 
} pc;

void main() {
    SceneData scene = allSceneBuffers[pc.sceneDataIndex].scene;
    
    mat4 shadowMatrix = scene.lightSpaceMatrix;
    if (pc.cascadeIndex < 4) {
        shadowMatrix = scene.cascadeViewProj[pc.cascadeIndex];
    }
    
    gl_Position = shadowMatrix * pc.model * vec4(inPos, 1.0);
}
