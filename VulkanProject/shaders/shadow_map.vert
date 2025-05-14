#version 450

layout(location = 0) in vec3 inPosition;

// Push constants for lightView and lightProj
layout(push_constant) uniform ShadowPushConstants {
    mat4 lightView;
    mat4 lightProj;
} pc;

void main() {
    gl_Position = pc.lightProj * pc.lightView * vec4(inPosition, 1.0);
}
