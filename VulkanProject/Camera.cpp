#include "Camera.h"
#include <algorithm>
#include <spdlog/spdlog.h>
Camera::Camera(GLFWwindow* window, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_Window(window), m_Position(position), m_WorldUp(up), m_Yaw(yaw), m_Pitch(pitch),
      m_FirstMouse(true), m_MovementSpeed(2.5f), m_MouseSensitivity(0.1f),
      m_DebugMode(0), m_F10Pressed(false), m_F2Pressed(false), m_F1Pressed(false),
      m_IPressedLast(false), m_KPressedLast(false), m_OPressedLast(false), m_LPressedLast(false),
      m_IblIntensity(1.0f), m_SunIntensity(100.0f)
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

    // Add F10 toggle for debug modes
    if (glfwGetKey(m_Window, GLFW_KEY_F10) == GLFW_PRESS) {
        if (!m_F10Pressed) {
            m_DebugMode = (m_DebugMode + 1) % 3; // Cycle through modes 0, 1, 2
            if (m_DebugMode == 0) {
                spdlog::info("Debug mode: Normal view");
            } else if (m_DebugMode == 1) {
                spdlog::info("Debug mode: Position check view");
            } else if (m_DebugMode == 2) {
                spdlog::info("Debug mode: rainbow view");
			}
            m_F10Pressed = true;
        }
    } else {
        m_F10Pressed = false;
    }

    // F2 cycles through G-buffer modes 3-6
    if (glfwGetKey(m_Window, GLFW_KEY_F2) == GLFW_PRESS) {
        if (!m_F2Pressed) {
            if (m_DebugMode < 3 || m_DebugMode > 6) {
                m_DebugMode = 3; // Start at diffuse view
				spdlog::info("Debug mode: Diffuse view");
            } else {
                m_DebugMode = (m_DebugMode + 1) % 7;
                if (m_DebugMode < 3) m_DebugMode = 3; // Keep in G-buffer range
                if (m_DebugMode == 3) {
                    spdlog::info("Debug mode: Diffuse view");
                } else if (m_DebugMode == 4) {
                    spdlog::info("Debug mode: Normal view");
                } else if (m_DebugMode == 5) {
                    spdlog::info("Debug mode: Specular view");
                } else if (m_DebugMode == 6) {
                    spdlog::info("Debug mode: worldpos view");
				}

            }
            m_F2Pressed = true;
        }
    } else {
        m_F2Pressed = false;
    }

    // F1 resets to normal view
    if (glfwGetKey(m_Window, GLFW_KEY_F1) == GLFW_PRESS) {
        if (!m_F1Pressed) {
            m_DebugMode = 0;
			spdlog::info("Debug mode: Normal view");
            m_F1Pressed = true;
        }
    } else {
        m_F1Pressed = false;
    }

    // Handle IBL intensity adjustment (I/K)
    if (glfwGetKey(m_Window, GLFW_KEY_I) == GLFW_PRESS) {
        if (!m_IPressedLast) {
            m_IblIntensity += 0.25f;
			spdlog::info("IBL Intensity increased to {}", m_IblIntensity);
            m_IPressedLast = true;
        }
    } else {
        m_IPressedLast = false;
    }
    
    if (glfwGetKey(m_Window, GLFW_KEY_K) == GLFW_PRESS) {
        if (!m_KPressedLast) {
            m_IblIntensity = std::max(0.0f, m_IblIntensity - 0.25f);
			spdlog::info("IBL Intensity decreased to {}", m_IblIntensity);
            m_KPressedLast = true;
        }
    } else {
        m_KPressedLast = false;
    }
    
    // Handle sun intensity adjustment (O/L)
    if (glfwGetKey(m_Window, GLFW_KEY_O) == GLFW_PRESS) {
        if (!m_OPressedLast) {
            m_SunIntensity *= 10.0f;
			spdlog::info("Sun Intensity increased to {}", m_SunIntensity);
            m_OPressedLast = true;
        }
    } else {
        m_OPressedLast = false;
    }
    
    if (glfwGetKey(m_Window, GLFW_KEY_L) == GLFW_PRESS) {
        if (!m_LPressedLast) {
            m_SunIntensity = std::max(0.1f, m_SunIntensity / 10.0f);
			spdlog::info("Sun Intensity decreased to {}", m_SunIntensity);
            m_LPressedLast = true;
        }
    } else {
        m_LPressedLast = false;
    }
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
