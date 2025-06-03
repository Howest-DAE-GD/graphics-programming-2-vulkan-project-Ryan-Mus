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

layout(binding = 9) uniform SunMatrices {
    mat4 lightProj;
    mat4 lightView;
} sunMatrices;

layout(push_constant) uniform PushConstants {
    int debugMode;
    float iblIntensity;
    float sunIntensity;
    float padding;
} pushConstants;

const float PI = 3.14159265359;

const vec3 sunDirection = normalize(vec3(-0.2, -1.0, -0.4));
const vec3 sunColor = vec3(1.0, 0.95, 0.8); // "Sunshine" color
//const float sunIntensity = 100.0; // 100 lux

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

void main() 
{
    ivec2 texelCoord = ivec2(fragTexCoord * ubo.viewportSize);
    const float depth = texelFetch(depthSampler, texelCoord, 0).r;
    const vec3 worldPos = reconstructWorldPosition(depth, fragTexCoord);

    // Sample all G-buffer textures we might need
    const vec4 albedoTexture = texture(diffuseSampler, fragTexCoord);
    const vec3 albedo = albedoTexture.rgb;
    const vec3 normalMap = normalize(texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0);
    const vec3 N = normalMap;
    const vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);
    const float metallic = metallicRoughness.b;
    const float roughness = metallicRoughness.g;
    
    // Single switch statement for all debug modes
    switch(pushConstants.debugMode) {
        case 0: { // Normal PBR rendering
            // Handle skybox
            if (depth >= 1.0) 
            {
                vec3 sampleDirection = normalize(worldPos.xyz);
                sampleDirection.y = -sampleDirection.y; 
                outColor = texture(skyboxSampler, sampleDirection);
                return;
            }
            vec3 V = normalize(ubo.cameraPosition - worldPos);
            vec3 F0 = vec3(0.04); 
            F0 = mix(F0, albedo, metallic);
            
            vec3 Lo = vec3(0.0);
            vec3 irradiance = texture(irradianceSampler, N).rgb;

            // Sun light calculation
            vec3 L = normalize(-sunDirection);
            float NdotL = max(dot(N, L), 0.0);
            
            if (NdotL > 0.0) {
                // Shadow mapping calculation
                vec4 lightSpacePosition = sunMatrices.lightProj * sunMatrices.lightView * vec4(worldPos, 1.0);
                lightSpacePosition /= lightSpacePosition.w;
                vec3 shadowMapUV = lightSpacePosition.xyz * 0.5 + 0.5;
                shadowMapUV.z = lightSpacePosition.z;

                ivec2 shadowMapSize = textureSize(shadowMapSampler, 0);
                float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);

                // PCF filtering
                float shadowTerm = 0.0;
                int kernelSize = 1;
                int samples = 0;
                for (int x = -kernelSize; x <= kernelSize; ++x) {
                    for (int y = -kernelSize; y <= kernelSize; ++y) {
                        vec2 offset = vec2(x, y) / vec2(shadowMapSize);
                        vec2 sampleUV = shadowMapUV.xy + offset;
                        float sampleDepth = texture(shadowMapSampler, sampleUV).r;
                        if (shadowMapUV.z <= sampleDepth + bias)
                            shadowTerm += 1.0;
                        samples++;
                    }
                }
                shadowTerm /= float(samples);

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
                
                // Use push constant sunIntensity instead of constant
                vec3 radiance = sunColor * pushConstants.sunIntensity;
                
                Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadowTerm;
            }
            
            // Point lights calculation
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
            
            // Ambient lighting with IBL intensity adjustment
            vec3 F_ambient = FresnelSchlick(max(dot(N, V), 0.0), F0);
            vec3 kS = F_ambient;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;
            
            vec3 diffuse = irradiance * albedo;
            // Multiply ambient by the IBL intensity from push constants
            vec3 ambient = kD * diffuse * pushConstants.iblIntensity;
            
            outColor = vec4(ambient + Lo, 1.0);
            break;
        }
        
        case 1: { // World grid visualization
            vec3 gridColor = visualizeWorldSpaceGrid(worldPos);
            
            // Show camera and light indicators
            if (fragTexCoord.x < 0.2 && fragTexCoord.y < 0.1) {
                vec3 camPosNormalized = normalize(ubo.cameraPosition) * 0.5 + 0.5;
                gridColor = camPosNormalized;
            }
            
            if (fragTexCoord.x > 0.8 && fragTexCoord.y < 0.1) {
                if (lightCount > 0) {
                    vec3 lightPosNormalized = normalize(lights[0].position) * 0.5 + 0.5;
                    gridColor = lightPosNormalized;
                } else {
                    gridColor = vec3(1.0, 0.0, 0.0);
                }
            }
            
            outColor = vec4(gridColor, 1.0);
            break;
        }
        
        case 2: // Position visualization
            outColor = vec4(normalize(worldPos) * 0.5 + 0.5, 1.0);
            break;
            
        case 3: // Diffuse/albedo buffer
            outColor = vec4(albedo, 1.0);
            break;
            
        case 4: // Normal buffer (convert from [-1,1] to [0,1] for display)
            outColor = vec4(normalMap * 0.5 + 0.5, 1.0);
            break;
            
        case 5: // Metallic (blue) and roughness (green) buffer
            outColor = vec4(0.0, metallicRoughness.g, metallicRoughness.b, 1.0);
            break;
            
        case 6: // worldpos
            outColor = vec4(vec3(worldPos), 1.0);
            break;
    }
}
