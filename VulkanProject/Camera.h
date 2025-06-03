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
    
    int getDebugMode() const { return m_DebugMode; }
    float getIblIntensity() const { return m_IblIntensity; }
    float getSunIntensity() const { return m_SunIntensity; }

    // Exposure settings struct
    struct ExposureSettings {
        float aperture;
        float ISO;
        float shutterSpeed;
    };
    
    // Getter for exposure settings
    ExposureSettings getExposureSettings() const { return m_ExposureSettings; }

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
    
    // Debug visualization
    int m_DebugMode = 0;
    bool m_F10Pressed = false;
    bool m_F2Pressed = false;
    bool m_F1Pressed = false;
    
    // Lighting controls
    float m_IblIntensity = 1.0f;
    float m_SunIntensity = 100.0f;
    bool m_IPressedLast = false;
    bool m_KPressedLast = false;
    bool m_OPressedLast = false;
    bool m_LPressedLast = false;

    // Camera exposure settings (default to INDOOR values)
    ExposureSettings m_ExposureSettings = {
        1.4f,   // aperture
        1600.0f, // ISO
        1.0f / 60.0f // shutterSpeed
    };
    
    // Key tracking for exposure controls
    bool m_UPressedLast = false;
    bool m_JPressedLast = false;
};
