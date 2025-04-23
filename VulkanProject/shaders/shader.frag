#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int fragTexIndex; // Received texture index

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSamplers[]; // Descriptor array of samplers

void main() {
    // Use the texture index to sample from the correct texture
    vec4 texColor = texture(texSamplers[fragTexIndex], fragTexCoord); // Sample with alpha
    
    const float alphaThreshold = 0.5; // Set your desired alpha threshold

    // Discard fragment if alpha is below threshold
    if (texColor.a < alphaThreshold) {
        discard;
    }
    
    outColor = vec4(fragColor * texColor.rgb, 0); // Use texture alpha
}
