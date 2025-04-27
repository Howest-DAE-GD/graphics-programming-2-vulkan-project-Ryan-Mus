#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
    Camera(GLFWwindow* window, glm::vec3 position, glm::vec3 up, float yaw, float pitch);

    glm::mat4 getViewMatrix();

    void update(float deltaTime);
	glm::vec3 getPosition() const { return m_Position; }

private:
    void processKeyboard(float deltaTime);
    void processMouse();

    GLFWwindow* m_Window;

    glm::vec3 m_Position;
    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    // Euler Angles
    float m_Yaw;
    float m_Pitch;

    // Mouse control
    bool m_FirstMouse;
    float m_LastX;
    float m_LastY;

    // Settings
    float m_MovementSpeed;
    float m_MouseSensitivity;
};
