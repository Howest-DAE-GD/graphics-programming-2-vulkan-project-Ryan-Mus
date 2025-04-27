// Frustum.h
#include <glm/glm.hpp>
#include <array>

class Frustum {
public:
    Frustum(const glm::mat4& projection, const glm::mat4& view);
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;
private:
    std::array<glm::vec4, 6> planes;
};
