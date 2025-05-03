#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPosition;

    fragTexCoord = inTexCoord;
    fragWorldPos = worldPosition.xyz;

    // Compute normal matrix for transforming normals correctly
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));

    // Transform and normalize each component
    fragNormal = normalize(normalMatrix * inNormal);                    // Use normal matrix for normal
    fragTangent = normalize(mat3(ubo.model) * inTangent);              // Use model matrix for tangent
    fragBitangent = normalize(mat3(ubo.model) * inBitangent);          // Use model matrix for bitangent
}
