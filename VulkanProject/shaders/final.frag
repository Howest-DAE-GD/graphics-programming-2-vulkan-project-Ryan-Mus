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

const float PI = 3.14159265359;

// Hardcoded omni light properties - FIXED WORLD SPACE POSITION
const vec3 lightPosition = vec3(0.0, 2.0, 0.0);  // Position in world space
const vec3 lightColor = vec3(1.0, 0.9, 0.7);     // Warm white color
const float lightIntensity = 1.0;                // Higher intensity for point light
const float lightRadius = 10.0;                  // Controls light falloff distance

// PBR functions
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

// Calculate attenuation for point light
float calculateAttenuation(float distance, float radius)
{
    // Inverse square law with smooth falloff at radius edge
    float att = clamp(1.0 - (distance * distance) / (radius * radius), 0.0, 1.0);
    return att * att;
}

// ACES tonemapping for better visual results
vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Reconstruct world position from depth - FIXED FUNCTION
vec3 reconstructWorldPosition(float depth, vec2 texCoord)
{
    // Convert from depth buffer value to NDC depth (-1 to 1)
    float z = depth * 2.0 - 1.0;
    
    // Create NDC position (x and y are from -1 to 1)
    vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, z, 1.0);
    
    // Transform from clip space to view space
    vec4 viewSpacePosition = inverse(ubo.proj) * clipSpacePosition;
    
    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;
    
    // Transform from view space to world space
    vec4 worldSpacePosition = inverse(ubo.view) * viewSpacePosition;
    
    return worldSpacePosition.xyz;
}

void main() 
{
    // Get albedo color from diffuse sampler
    vec4 albedoTexture = texture(diffuseSampler, fragTexCoord);
    vec3 albedo = albedoTexture.rgb;
    
    // Get normal from normal map and transform to world space
    vec3 normalMap = normalize(texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0);
    
    // For proper world-space normal calculation, we would need the TBN matrix
    // As a temporary solution, we'll use the normal as-is, assuming it's already in world space
    vec3 N = normalMap;
    
    // Get metallic and roughness values from texture
    vec4 metallicRoughness = texture(metallicRoughnessSampler, fragTexCoord);
    float metallic = metallicRoughness.b;
    float roughness = metallicRoughness.g;
    
    // Get depth value using TexelFetch to avoid filtering
    //ivec2 texelCoord = ivec2(fragTexCoord * ubo.viewportSize);
    float depth = texelFetch(depthSampler, ivec2(fragTexCoord.xy), 0).r;
    
    // Reconstruct world position
    vec3 worldPos = reconstructWorldPosition(depth, fragTexCoord);
    
    // View direction
    vec3 V = normalize(ubo.cameraPosition - worldPos);
    
    // Calculate reflectance at normal incidence (F0)
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
    // Calculate light direction and distance - Using fixed world-space light position
    vec3 L = normalize(lightPosition - worldPos);
    float distance = length(lightPosition - worldPos);
    float attenuation = calculateAttenuation(distance, lightRadius);
    vec3 radiance = lightColor * lightIntensity * attenuation;
    
    // Half vector
    vec3 H = normalize(V + L);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;
    
    // Add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    
    // Ambient lighting with occlusion consideration
    float ao = 1.0; // If you have ambient occlusion, use it here
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    // Final color
    vec3 color = ambient + Lo;
    
    // ACES Filmic Tone mapping (HDR -> LDR)
    color = ACESFilm(color);
    
    // Gamma correction
    //color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, albedoTexture.a);
}