#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMetallicRoughness;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPosition; // Moved cameraPosition here
} ubo;

layout(binding = 1) uniform sampler2D diffuseSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D metallicRoughnessSampler;

void main() {
    // Sample textures
    vec4 diffuseColor = texture(diffuseSampler, fragTexCoord);
    vec3 normalMap = normalize(texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0); // Convert to [-1,1] range and normalize
    vec4 metallicRoughnessColor = texture(metallicRoughnessSampler, fragTexCoord);

    if (diffuseColor.a < 0.5) {
        discard; // Discard fragments with low alpha
    }

    // Construct TBN matrix
    vec3 T = normalize(fragTangent);
    vec3 B = normalize(fragBitangent);
    vec3 N = normalize(fragNormal);
    mat3 TBN = mat3(T, B, N);

    // Transform normal from tangent space to world space
    vec3 worldNormal = normalize(TBN * normalMap);

    // Encode normal into [0,1] range for output
    outNormal = vec4(worldNormal * 0.5 + 0.5, 1.0);

    outColor = diffuseColor;
    outMetallicRoughness = metallicRoughnessColor; // Pass metallic and roughness values to the output
}
