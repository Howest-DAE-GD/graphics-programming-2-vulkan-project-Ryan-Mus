#version 450

layout(location = 0) in vec3 fragLocalPosition;

// Descriptor Binding for HDRI Texture
layout(binding = 0) uniform sampler2D sphericalMap; // Spherical HDRI map

layout(location = 0) out vec4 outColor;

void main() {
    // Normalize the local position to get a direction vector
    vec3 direction = normalize(fragLocalPosition);

    // Convert the direction vector to spherical coordinates
    float theta = atan(direction.z, direction.x); // Azimuth angle (longitude)
    float phi = acos(direction.y);               // Elevation angle (latitude)

    // Map spherical coordinates to UV coordinates
    // Ensure +Y is up and map theta and phi to [0, 1] range
    float u = (theta / (2.0 * 3.14159265359)) + 0.5; // Map theta from [-PI, PI] to [0, 1]
    float v = phi / 3.14159265359;                  // Map phi from [0, PI] to [0, 1]

    // Sample the spherical map using the UV coordinates
    vec3 hdrColor = texture(sphericalMap, vec2(u, v)).rgb;

    // Output the HDRI color
    outColor = vec4(hdrColor, 1.0);
}
