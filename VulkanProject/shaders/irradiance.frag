#version 450

layout(location = 0) in vec3 fragLocalPosition;

// Descriptor Binding for HDRI Texture
layout(binding = 0) uniform samplerCube sphericalMap; // Spherical HDRI map

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

void main() {
    vec3 N = normalize(fragLocalPosition);

    vec3 irradiance = vec3(0.0);
    float sampleDelta = 0.025;
    int sampleCount = 0;

    // Tangent space basis
    vec3 up = vec3(0.0, -1.0, 0.0);

    // Flip the up vector if the dominant axis of N is negative
    if (N.y < 0.0) {
        up = -up;
    }

    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
    mat3 TBN = mat3(right, up, N);

    // Fixed-step convolution over the hemisphere
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // Spherical to Cartesian coordinates (in tangent space)
            vec3 tangentSample = vec3(
                sin(theta) * cos(phi),
                cos(theta),
                sin(theta) * sin(phi)
            );

            // Transform to world space
            vec3 sampleVec = TBN * tangentSample;

            irradiance += texture(sphericalMap, sampleVec).rgb * cos(theta) * sin(theta);
            sampleCount++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(sampleCount));

    outColor = vec4(irradiance, 1.0);
}
