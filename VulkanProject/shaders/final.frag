#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D diffuseSampler; 
layout(binding = 1) uniform sampler2D normalSampler;
layout(binding = 2) uniform sampler2D metallicRoughnessSampler;
layout(binding = 3) uniform sampler2D depthSampler;

layout(binding = 4) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPosition;
    vec2 viewportSize;
} ubo;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

layout(std430, binding = 5) readonly buffer LightsBuffer {
    uint lightCount;
    Light lights[];
};

layout(binding = 6) uniform samplerCube skyboxSampler;
layout(binding = 7) uniform samplerCube irradianceSampler;

layout(binding = 8) uniform sampler2D shadowMapSampler;

layout(push_constant) uniform SunMatrices {
    mat4 lightProj;
    mat4 lightView;
} sunMatrices;

const float PI = 3.14159265359;

const vec3 sunDirection = normalize(vec3(-0.2, -1.0, -0.4));
const vec3 sunColor = vec3(1.0, 0.95, 0.8); // "Sunshine" color
const float sunIntensity = 100.0; // 100 lux

// Debug visualization mode - switch between different tests
#define DEBUG_MODE 0 // 0=normal render, 1=world pos, 2=debug view

// PBR functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness, bool isIndirect) {
    float r = (roughness + 1.0);
    float k = isIndirect ? (r * r) / 2.0 : (r * r) / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness, bool isIndirect) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness, isIndirect);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness, isIndirect);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float calculateAttenuation(float distance, float radius) {
    float att = clamp(1.0 - (distance * distance) / (radius * radius), 0.0, 1.0);
    return att * att;
}

// Vulkan-specific world position reconstruction
vec3 reconstructWorldPosition(float depth, vec2 texCoord) {
    vec4 clipPos = vec4(texCoord * 2.0 - 1.0, depth, 1.0);
    mat4 invViewProj = inverse(ubo.proj * ubo.view);
    vec4 worldPosH = invViewProj * clipPos;
    return worldPosH.xyz / worldPosH.w;
}

// Grid visualization function
vec3 visualizeWorldSpaceGrid(vec3 worldPos) {
    float gridSize = 1.0;
    float lineWidth = 0.05; 
    vec3 color = vec3(0.0);

    float xGrid = abs(mod(worldPos.x, gridSize));
    if (xGrid < lineWidth || xGrid > gridSize - lineWidth)
        color.r = 1.0;

    float yGrid = abs(mod(worldPos.y, gridSize));
    if (yGrid < lineWidth || yGrid > gridSize - lineWidth)
        color.g = 1.0;

    float zGrid = abs(mod(worldPos.z, gridSize));
    if (zGrid < lineWidth || zGrid > gridSize - lineWidth)
        color.b = 1.0;

    float distToOrigin = length(worldPos);
    if (distToOrigin < 0.1)
        return vec3(1.0);

    if (lightCount > 0) {
        float minDist = 1000.0;
        int closestLightIndex = 0;

        for (uint i = 0; i < lightCount; i++) {
            float lightDist = length(worldPos - lights[i].position);
            if (lightDist < minDist) {
                minDist = lightDist;
                closestLightIndex = int(i);
            }
        }

        float lightDist = length(worldPos - lights[closestLightIndex].position);
        if (lightDist < 0.2)
            return lights[closestLightIndex].color;
    }

    return color;
}

void main() {
    ivec2 texelCoord = ivec2(fragTexCoord * ubo.viewportSize);
    const float depth = texelFetch(depthSampler, texelCoord, 0).r;

    const vec3 worldPos = reconstructWorldPosition(depth, fragTexCoord);
     
    if (depth >= 1.0) {
        vec3 sampleDirection = normalize(worldPos.xyz);
        sampleDirection.y = -sampleDirection.y; 
        outColor = texture(skyboxSampler, sampleDirection);
        return;
    }

    const vec4 albedoTexture = texture(diffuseSampler, fragTexCoord);
    const vec3 albedo = albedoTexture.rgb;
    
    const vec3 normalMap = normalize(texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0);
    const vec3 N = normalMap;
   
    vec3 irradiance = texture(irradianceSampler, N).rgb;

    const vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);
    const float metallic = metallicRoughness.b;
    const float roughness = metallicRoughness.g;
    
    vec3 finalColor;
    
    if (DEBUG_MODE == 0) 
    {
        vec3 V = normalize(ubo.cameraPosition - worldPos);
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metallic);
        
        vec3 Lo = vec3(0.0);

        vec3 L = normalize(-sunDirection);
        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0) 
        {
            vec4 lightSpacePosition = sunMatrices.lightProj * sunMatrices.lightView * vec4(worldPos, 1.0);
            lightSpacePosition /= lightSpacePosition.w;
            vec3 shadowMapUV = lightSpacePosition.xyz * 0.5 + 0.5;
            shadowMapUV.z = lightSpacePosition.z; // Use z for depth comparison
            //shadowMapUV.y = 1.0 - shadowMapUV.y;

            // Manual shadow comparison
            ivec2 shadowMapSize = textureSize(shadowMapSampler, 0);
            ivec2 shadowTexel = ivec2(shadowMapUV.xy * shadowMapSize);
            float shadowMapDepth = texelFetch(shadowMapSampler, shadowTexel, 0).r;
            float lightDepth = shadowMapUV.z;
            float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);

            float shadowTerm = 1.0;
            if (lightDepth > shadowMapDepth + bias)
                shadowTerm = 0.0;

            vec3 H = normalize(V + L);

            float NDF = DistributionGGX(N, H, roughness);
            float G   = GeometrySmith(N, V, L, roughness, false);
            vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;

            vec3 numerator    = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
            vec3 specular     = numerator / denominator;

            vec3 radiance = sunColor * sunIntensity;

            Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadowTerm;
        } 
        // Point lights
        for (uint i = 0; i < lightCount; i++) {
            Light light = lights[i];
            vec3 L = normalize(light.position - worldPos);
            float distance = length(light.position - worldPos);
            float attenuation = calculateAttenuation(distance, light.radius);
            vec3 radiance = light.color * light.intensity * attenuation;
            
            vec3 H = normalize(V + L);
            
            float NDF = DistributionGGX(N, H, roughness);
            float G   = GeometrySmith(N, V, L, roughness, false);
            vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
            
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;
            
            vec3 numerator    = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular     = numerator / denominator;
            
            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }

        // Ambient lighting
        vec3 F_ambient = FresnelSchlick(max(dot(N, V), 0.0), F0);
        vec3 kS = F_ambient;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 diffuse = irradiance * albedo;
        vec3 ambient = kD * diffuse * 1.0;

        finalColor = ambient + Lo;
    }
    else if (DEBUG_MODE == 1) 
    {
        finalColor = visualizeWorldSpaceGrid(worldPos);

        if (fragTexCoord.x < 0.2 && fragTexCoord.y < 0.1) {
            vec3 camPosNormalized = normalize(ubo.cameraPosition) * 0.5 + 0.5;
            finalColor = camPosNormalized;
        }

        if (fragTexCoord.x > 0.8 && fragTexCoord.y < 0.1) {
            if (lightCount > 0) {
                vec3 lightPosNormalized = normalize(lights[0].position) * 0.5 + 0.5;
                finalColor = lightPosNormalized;
            } else {
                finalColor = vec3(1.0, 0.0, 0.0);
            }
        }
    }
    else if (DEBUG_MODE == 2) {
        finalColor = normalize(worldPos) * 0.5 + 0.5;
    }
    outColor = vec4(finalColor, 1.0);
}
