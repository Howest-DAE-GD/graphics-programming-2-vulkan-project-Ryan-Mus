#version 450

// Push Constants for View and Projection Matrices
layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 projection;
} pushConstants;

// Hardcoded vertices for a unit cube
const vec3 g_vertexPositions[36] = vec3[](
    // +X
    vec3(1,  1, -1), vec3(1, -1, -1), vec3(1, -1,  1),
    vec3(1, -1,  1), vec3(1,  1,  1), vec3(1,  1, -1),

    // -X
    vec3(-1,  1, -1), vec3(-1, -1, -1), vec3(-1, -1,  1),
    vec3(-1, -1,  1), vec3(-1,  1,  1), vec3(-1,  1, -1),

    // +Y
    vec3(-1,  1, -1), vec3( 1,  1, -1), vec3( 1,  1,  1),
    vec3( 1,  1,  1), vec3(-1,  1,  1), vec3(-1,  1, -1),

    // -Y
    vec3(-1, -1, -1), vec3( 1, -1, -1), vec3( 1, -1,  1),
    vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, -1, -1),

    // +Z
    vec3(-1,  1,  1), vec3( 1,  1,  1), vec3( 1, -1,  1),
    vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),

    // -Z
    vec3(-1,  1, -1), vec3( 1,  1, -1), vec3( 1, -1, -1),
    vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1)
);

// Output to Fragment Shader
layout(location = 0) out vec3 fragLocalPosition;

void main() {
    // Pass the local position of the vertex to the fragment shader
    fragLocalPosition = g_vertexPositions[gl_VertexIndex];

    // Transform the vertex position using the view and projection matrices
    vec4 worldPosition = vec4(fragLocalPosition, 1.0);
    gl_Position = pushConstants.projection * pushConstants.view * worldPosition;
}
