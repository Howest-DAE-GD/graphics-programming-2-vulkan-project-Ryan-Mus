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

layout(std430,binding = 5) readonly buffer LightsBuffer {
    uint lightCount;
    Light lights[];
};

layout(binding = 6) uniform samplerCube skyboxSampler;

const float PI = 3.14159265359;

// Debug visualization mode - switch between different tests
#define DEBUG_MODE 0  // 0=normal render, 1=world pos, 2=debug view

// Unaltered PBR functions from original shader
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float calculateAttenuation(float distance, float radius)
{
    float att = clamp(1.0 - (distance * distance) / (radius * radius), 0.0, 1.0);
    return att * att;
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Vulkan-specific world position reconstruction
vec3 reconstructWorldPosition(float depth, vec2 texCoord)
{
    // For Vulkan's coordinate system
    vec4 clipPos = vec4(texCoord * 2.0 - 1.0, depth, 1.0);
    
    // Get inverse view-projection matrix
    mat4 invViewProj = inverse(ubo.proj * ubo.view);
    
    // Transform from clip space to world space
    vec4 worldPosH = invViewProj * clipPos;
    
    // Perspective division
    return worldPosH.xyz / worldPosH.w;
}

// Alternative world position reconstruction - try this if the above doesn't work
vec3 reconstructWorldPosition_Alt(float depth, vec2 texCoord)
{
    // Convert UV to NDC (-1 to 1)
    vec2 ndcXY = (texCoord * 2.0 - 1.0);
    
    // Create ray from camera position through pixel
    // Vulkan uses reverse Z depth, so we need to handle that
    vec4 clipPos = vec4(ndcXY.x, ndcXY.y, depth, 1.0);
    
    // Get inverse view projection
    mat4 invViewProj = inverse(ubo.proj * ubo.view);
    
    // Transform to world space
    vec4 worldPosH = invViewProj * clipPos;
    
    // Perspective division
    vec3 worldPos = worldPosH.xyz / worldPosH.w;
    
    return worldPos;
}

// Grid visualization function
vec3 visualizeWorldSpaceGrid(vec3 worldPos)
{
    // Visualize with a fixed grid
    float gridSize = 1.0;
    float lineWidth = 0.05; 
    
    // Basic grid visualization
    vec3 color = vec3(0.0);
    
    // Grid lines for X axis (red component)
    float xGrid = abs(mod(worldPos.x, gridSize));
    if (xGrid < lineWidth || xGrid > gridSize - lineWidth)
        color.r = 1.0;
    
    // Grid lines for Y axis (green component)
    float yGrid = abs(mod(worldPos.y, gridSize));
    if (yGrid < lineWidth || yGrid > gridSize - lineWidth)
        color.g = 1.0;
    
    // Grid lines for Z axis (blue component)
    float zGrid = abs(mod(worldPos.z, gridSize));
    if (zGrid < lineWidth || zGrid > gridSize - lineWidth)
        color.b = 1.0;
    
    // Highlight origin
    float distToOrigin = length(worldPos);
    if (distToOrigin < 0.1)
        return vec3(1.0); // White at origin
        
    // Add distance to light indication - now visualize closest light
    if (lightCount > 0) {
        // Find closest light
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
            return lights[closestLightIndex].color; // Show light color at light position
    }
        
    return color;
}

void main() 
{
    // Get depth value using texelFetch for raw depth
    ivec2 texelCoord = ivec2(fragTexCoord * ubo.viewportSize);
    const float depth = texelFetch(depthSampler, texelCoord, 0).r;

    const vec3 worldPos = reconstructWorldPosition(depth, fragTexCoord);
     
     if (depth >= 1.0) {

        vec3 sampleDirection = normalize(worldPos.xyz);

        sampleDirection.y = -sampleDirection.y; 
        
        outColor = texture(skyboxSampler, sampleDirection);
        return;
     }

    // Get material properties from textures
    const vec4 albedoTexture = texture(diffuseSampler, fragTexCoord);
    const vec3 albedo = albedoTexture.rgb;
    
    const vec3 normalMap = normalize(texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0);
    const vec3 N = normalMap;
    
    const vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);
    const float metallic = metallicRoughness.b;
    const float roughness = metallicRoughness.g;
    
    vec3 finalColor;
    
    if (DEBUG_MODE == 0) {
        // Standard PBR lighting - now using lights array from binding 5
        vec3 V = normalize(ubo.cameraPosition - worldPos);
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metallic);
        
        vec3 Lo = vec3(0.0);
        
        // Process all lights in the light buffer
        for (uint i = 0; i < lightCount; i++) {
            Light light = lights[i];
            
            vec3 L = normalize(light.position - worldPos);
            float distance = length(light.position - worldPos);
            float attenuation = calculateAttenuation(distance, light.radius);
            vec3 radiance = light.color * light.intensity * attenuation;
            
            vec3 H = normalize(V + L);
            
            float NDF = DistributionGGX(N, H, roughness);
            float G   = GeometrySmith(N, V, L, roughness);
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
        
        vec3 ambient = vec3(0.03) * albedo;
        
        finalColor = ambient + Lo;
        //finalColor = ACESFilm(finalColor);
    }
    else if (DEBUG_MODE == 1) {
        // World position visualization with grid
        finalColor = visualizeWorldSpaceGrid(worldPos);
        
        // Show camera position in corner
        if (fragTexCoord.x < 0.2 && fragTexCoord.y < 0.1) {
            vec3 camPosNormalized = normalize(ubo.cameraPosition) * 0.5 + 0.5;
            finalColor = camPosNormalized;
        }
        
        // Show light count and first light position in opposite corner
        if (fragTexCoord.x > 0.8 && fragTexCoord.y < 0.1) {
            if (lightCount > 0) {
                vec3 lightPosNormalized = normalize(lights[0].position) * 0.5 + 0.5;
                finalColor = lightPosNormalized;
            } else {
                // Red if no lights
                finalColor = vec3(1.0, 0.0, 0.0);
            }
        }
    }
    else if (DEBUG_MODE == 2) {
        // Debug visualization - show raw components
        
        // Top third: raw depth values
        if (fragTexCoord.y < 0.33) {
            finalColor = vec3(depth);
        }
        // Middle third: camera space positions
        else if (fragTexCoord.y < 0.66) {
            vec4 clipPos = vec4(fragTexCoord * 2.0 - 1.0, depth, 1.0);
            vec4 viewPos = inverse(ubo.proj) * clipPos;
            viewPos /= viewPos.w;
            finalColor = normalize(viewPos.xyz) * 0.5 + 0.5;
        }
        // Bottom third: world positions as colors
        else {
            // Normalize world position to a visible color range
            finalColor = normalize(worldPos) * 0.5 + 0.5;
        }
        
        // Show coordinate cross
        float lineWidth = 0.005;
        if (abs(fragTexCoord.x - 0.5) < lineWidth || abs(fragTexCoord.y - 0.5) < lineWidth) {
            finalColor = vec3(1.0);
        }
        
        // Show light count in bottom right corner
        if (fragTexCoord.x > 0.9 && fragTexCoord.y > 0.9) {
            // Visualize number of lights (green for lights, red for no lights)
            finalColor = lightCount > 0 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        }
    }
    
    outColor = vec4(finalColor, 1.0);
}
