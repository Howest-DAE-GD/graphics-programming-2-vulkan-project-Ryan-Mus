#version 450

layout(location = 0) in vec3 fragLocalPosition;
layout(binding = 0) uniform samplerCube sphericalMap;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
const uint SAMPLE_COUNT = 1024u;

// Radical inverse for Hammersley sequence
float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Hammersley point set
vec2 hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), radicalInverse_VdC(i));
}

// Cosine-weighted hemisphere sampling
vec3 sampleHemisphere(vec2 xi) {
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt(1.0 - xi.y);
    float sinTheta = sqrt(xi.y);

    return vec3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta
    );
}

void main() {
    vec3 N = normalize(fragLocalPosition);

    // Robust tangent basis
    vec3 up = vec3(0.0, -1.0, 0.0);
    if (N.y < 0.0) {
        up = -up;
    }
    vec3 right = normalize(cross(up, N));
    vec3 tangentUp = cross(N, right);
    mat3 TBN = mat3(right, tangentUp, N);

    vec3 irradiance = vec3(0.0);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 xi = hammersley(i, SAMPLE_COUNT);
        vec3 localDir = sampleHemisphere(xi);
        vec3 worldDir = normalize(TBN * localDir);

        float NdotL = max(dot(N, worldDir), 0.0);
        irradiance += texture(sphericalMap, worldDir).rgb * NdotL;
    }

    irradiance = PI * irradiance / float(SAMPLE_COUNT);

    outColor = vec4(irradiance, 1.0);
}
