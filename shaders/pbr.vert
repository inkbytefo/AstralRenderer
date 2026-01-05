#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in vec4 inColor;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outColor;
layout(location = 4) out vec4 outTangent;

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
    uint padding;
} pc;

void main() {
    SceneData scene = allSceneBuffers[pc.sceneDataIndex].scene;
    
    // glTF standard is right-handed, Y-up.
    // We'll apply a rotation to make it face the camera if needed, but identity for now.
    vec4 worldPos = pc.model * vec4(inPos, 1.0);
    outWorldPos = worldPos.xyz;
    
    // Normal matrix
    mat3 normalMatrix = mat3(transpose(inverse(pc.model)));
    outNormal = normalMatrix * inNormal;
    outTangent = vec4(normalMatrix * inTangent.xyz, inTangent.w);
    outUV = inUV;
    outColor = inColor;
    
    gl_Position = scene.viewProj * worldPos;
}
