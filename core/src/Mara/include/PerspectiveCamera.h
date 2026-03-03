#pragma once
#include "Camera.h"
#include <glm/glm.hpp>

class PerspectiveCamera : public Camera
{
public:
    PerspectiveCamera(float fov, float aspect, float nearPlane, float farPlane);

    // default constructor with typical values
    PerspectiveCamera()
        : PerspectiveCamera(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f) {}

    void Update(float deltaTime) override;

    void ProcessKeyboard(float deltaTime, bool forward, bool backward, bool left, bool right);
    void ProcessMouseMovement(float xoffset, float yoffset);

    void SetAspectRatio(float aspect);

    void MoveForward(float amount) { ProcessKeyboard(amount, false, false, false, false); }
    void MoveRight(float amount) { ProcessKeyboard(false, false, amount, false, false); }
    void Rotate(float xoffset, float yoffset) { ProcessMouseMovement(xoffset, yoffset); }

private:
    void UpdateView();
    void UpdateProjection();

    glm::vec3 m_Position;
    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    float m_Yaw;
    float m_Pitch;

    float m_FOV;
    float m_Aspect;
    float m_Near;
    float m_Far;

    float m_MovementSpeed;
    float m_MouseSensitivity;
};