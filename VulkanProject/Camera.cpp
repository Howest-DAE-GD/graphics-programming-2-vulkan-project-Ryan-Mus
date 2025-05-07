#include "Camera.h"

Camera::Camera(GLFWwindow* window, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_Window(window), m_Position(position), m_WorldUp(up), m_Yaw(yaw), m_Pitch(pitch),
      m_FirstMouse(true), m_MovementSpeed(2.5f), m_MouseSensitivity(0.1f)
{
    m_Front = glm::vec3(0.0f, 0.0f, 1.0f);
    update(0.0f);
}

void Camera::update(float deltaTime)
{
    processKeyboard(deltaTime);
    processMouse();

    // Update Front, Right, and Up Vectors using the updated Euler angles for Z-up
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);

    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}


glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(m_Position , m_Position + m_Front, m_Up);
}

void Camera::processKeyboard(float deltaTime)
{
    float velocity = m_MovementSpeed * deltaTime;
    if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
        m_Position += m_Front * velocity;
    if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
        m_Position -= m_Front * velocity;
    if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
        m_Position -= m_Right * velocity;
    if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
        m_Position += m_Right * velocity;
    if (glfwGetKey(m_Window, GLFW_KEY_SPACE) == GLFW_PRESS)
        m_Position += m_WorldUp * velocity;
    if (glfwGetKey(m_Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        m_Position -= m_WorldUp * velocity;
}


void Camera::processMouse()
{
    static bool rightMouseButtonPressed = false;

    if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        if (!rightMouseButtonPressed)
        {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            m_FirstMouse = true;
            rightMouseButtonPressed = true;
        }

        double xpos, ypos;
        glfwGetCursorPos(m_Window, &xpos, &ypos);

        if (m_FirstMouse)
        {
            m_LastX = xpos;
            m_LastY = ypos;
            m_FirstMouse = false;
        }

        float xoffset = xpos - m_LastX;
        float yoffset = m_LastY - ypos; // reversed since y-coordinates go from bottom to top

        m_LastX = xpos;
        m_LastY = ypos;

        xoffset *= m_MouseSensitivity;
        yoffset *= m_MouseSensitivity;

        m_Yaw += xoffset;
        m_Pitch += yoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (m_Pitch > 89.0f)
            m_Pitch = 89.0f;
        if (m_Pitch < -89.0f)
            m_Pitch = -89.0f;
    }
    else if (rightMouseButtonPressed)
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        rightMouseButtonPressed = false;
    }
}
