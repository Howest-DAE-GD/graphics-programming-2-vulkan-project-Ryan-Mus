// Frustum.cpp
#include "Frustum.h"

Frustum::Frustum(const glm::mat4& projection, const glm::mat4& view) {
    glm::mat4 clip = projection * view;

    // Extract planes from the combined matrix
    // Right plane
    planes[0] = glm::vec4(
        clip[0][3] - clip[0][0],
        clip[1][3] - clip[1][0],
        clip[2][3] - clip[2][0],
        clip[3][3] - clip[3][0]);
    // Left plane
    planes[1] = glm::vec4(
        clip[0][3] + clip[0][0],
        clip[1][3] + clip[1][0],
        clip[2][3] + clip[2][0],
        clip[3][3] + clip[3][0]);
    // Bottom plane
    planes[2] = glm::vec4(
        clip[0][3] + clip[0][1],
        clip[1][3] + clip[1][1],
        clip[2][3] + clip[2][1],
        clip[3][3] + clip[3][1]);
    // Top plane
    planes[3] = glm::vec4(
        clip[0][3] - clip[0][1],
        clip[1][3] - clip[1][1],
        clip[2][3] - clip[2][1],
        clip[3][3] - clip[3][1]);
    // Far plane
    planes[4] = glm::vec4(
        clip[0][3] - clip[0][2],
        clip[1][3] - clip[1][2],
        clip[2][3] - clip[2][2],
        clip[3][3] - clip[3][2]);
    // Near plane
    planes[5] = glm::vec4(
        clip[0][3] + clip[0][2],
        clip[1][3] + clip[1][2],
        clip[2][3] + clip[2][2],
        clip[3][3] + clip[3][2]);

    // Normalize the planes
    for (auto& plane : planes) {
        float length = glm::length(glm::vec3(plane));
        plane /= length;
    }
}

bool Frustum::isBoxVisible(const glm::vec3& min, const glm::vec3& max) const {
    for (const auto& plane : planes) {
        glm::vec3 positiveVertex = min;
        glm::vec3 negativeVertex = max;

        if (plane.x >= 0) {
            positiveVertex.x = max.x;
            negativeVertex.x = min.x;
        }
        if (plane.y >= 0) {
            positiveVertex.y = max.y;
            negativeVertex.y = min.y;
        }
        if (plane.z >= 0) {
            positiveVertex.z = max.z;
            negativeVertex.z = min.z;
        }

        if (glm::dot(glm::vec3(plane), positiveVertex) + plane.w < 0) {
            return false; // Outside the frustum
        }
    }
    return true; // Inside or intersects the frustum
}
