#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal; // Input normal

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragWorldPos; // Pass world position to fragment shader
layout(location = 2) out vec3 fragNormal;   // Pass normal to fragment shader

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0); // Calculate world position
    gl_Position = ubo.proj * ubo.view * worldPosition;
    fragTexCoord = inTexCoord;
    fragWorldPos = worldPosition.xyz; // Pass world position
    fragNormal = mat3(ubo.model) * inNormal; // Transform normal to world space
}
