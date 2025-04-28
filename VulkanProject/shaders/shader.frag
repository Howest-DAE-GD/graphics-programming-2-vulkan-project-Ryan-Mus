#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outSpecularColor;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPosition; // Moved cameraPosition here
} ubo;

layout(binding = 1) uniform sampler2D diffuseSampler;
layout(binding = 2) uniform sampler2D specularSampler;
layout(binding = 3) uniform sampler2D alphaSampler;

void main() {
    // Sample textures
    vec4 diffuseColor = texture(diffuseSampler, fragTexCoord);
    vec4 specularColor = texture(specularSampler, fragTexCoord);
    vec4 alphaColor = texture(alphaSampler, fragTexCoord);

    if( alphaColor.a < 0.5) {
        discard; // Discard fragments with low alpha
    }

    outSpecularColor = specularColor; // Pass specular color to the output
    outColor = diffuseColor;
}
