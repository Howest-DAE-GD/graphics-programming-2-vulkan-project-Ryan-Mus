#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragWorldPos; // Receive world position from vertex shader

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outSpecularColor; // Output specular color

layout(binding = 1) uniform sampler2D diffuseSampler;
layout(binding = 2) uniform sampler2D specularSampler;
layout(binding = 3) uniform sampler2D alphaSampler;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPosition; // Add camera position to the UBO
} ubo;

void main() {
    // Sample textures
    vec4 diffuseColor = texture(diffuseSampler, fragTexCoord);
    vec4 specularColor = texture(specularSampler, fragTexCoord);
    vec4 alphaColor = texture(alphaSampler, fragTexCoord);

    outSpecularColor = specularColor; // Pass specular color to the output
    
    const float alphaThreshold = 0.5; // Set your desired alpha threshold

    // Discard fragment if alpha is below threshold
    if (alphaColor.r < alphaThreshold) {
        discard;
    }

    // Hard-coded light direction (normalized)
    const vec3 lightDirection = normalize(vec3(1.0, -1.0, 1.0)); // Example: light coming from above

    // Calculate view direction using camera position and fragment world position
    vec3 viewDirection = normalize(ubo.cameraPosition - fragWorldPos);

    // Calculate normal vector (assume a flat surface with normal pointing up)
    vec3 normal = vec3(0.0, 0.0, 1.0);

    // Calculate reflection vector
    vec3 reflectDir = reflect(-lightDirection, normal);

    // Calculate specular intensity using the Phong reflection model
    float specularIntensity = pow(max(dot(reflectDir, viewDirection), 0.0), 32.0); // Shininess factor = 32

    // Combine diffuse and specular components
    vec3 finalColor = diffuseColor.rgb + specularColor.rgb * specularIntensity;

    outColor = vec4(finalColor, diffuseColor.a);
}
