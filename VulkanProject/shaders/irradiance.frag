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
    vec3 N_normal = normalize(fragLocalPosition);

    // Robust tangent basis construction
    vec3 tangent;
    vec3 bitangent;

    // Check if N_normal is aligned with the Y-axis
    if (abs(N_normal.y) > 0.999) {
        // N_normal is primarily along Y. Use a helper vector not parallel to Y (e.g., Z-axis or X-axis).
        // cross(N_normal, vec3(0.0, 0.0, 1.0))
        tangent = normalize(cross(N_normal, vec3(0.0, 0.0, 1.0))); 
        // Fallback if N_normal was somehow also aligned with Z_axis (e.g. (0,0,1) which isn't the N_normal.y > 0.999 case, but for safety)
        if (length(tangent) < 0.001) {
             tangent = normalize(cross(N_normal, vec3(1.0, 0.0, 0.0))); // Try X-axis
        }
    } else {
        // N_normal is not primarily along Y-axis, so world Y-up (0.0, 1.0, 0.0) is a safe helper.
        tangent = normalize(cross(vec3(0.0, 1.0, 0.0), N_normal));
    }
    // Bitangent is perpendicular to N_normal and tangent.
    // Normalizing here ensures it's a unit vector, important if N_normal and tangent are not perfectly orthogonal after normalization due to precision.
    bitangent = normalize(cross(N_normal, tangent)); 

    mat3 TBN = mat3(tangent, bitangent, N_normal);

    vec3 irradiance = vec3(0.0);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 xi = hammersley(i, SAMPLE_COUNT);
        vec3 localDir = sampleHemisphere(xi);         // Cosine-weighted sample in tangent space (local Z is normal)
        vec3 worldDir = normalize(TBN * localDir);   // Transform to world space oriented around N_normal

        // For cosine-weighted importance sampling, the NdotL (cos_theta) term
        // is part of the PDF and cancels out. The estimator becomes (PI / N_samples) * sum(L_i).
        // L_i is texture(sphericalMap, worldDir).rgb.
        irradiance += texture(sphericalMap, worldDir).rgb; // NdotL removed
    }

    irradiance = PI * irradiance / float(SAMPLE_COUNT);

    outColor = vec4(irradiance, 1.0);
}
