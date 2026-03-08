#include "PerspectiveCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>

PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float nearPlane, float farPlane)
    : m_FOV(fov), m_Aspect(aspect), m_Near(nearPlane), m_Far(farPlane),
      m_Position(0.0f, 0.0f, 12.0f), m_Front(0.0f, -0.2f, -1.0f),
      m_WorldUp(0.0f, 1.0f, 0.0f), m_Yaw(-90.0f), m_Pitch(10.0f),
      m_MovementSpeed(3.0f), m_MouseSensitivity(0.1f)
{
    UpdateView();
    UpdateProjection();
}

void PerspectiveCamera::Update(float deltaTime)
{
    // For now, just recompute matrices, will be called every frame by Renderer, update Later for optimization to only update when needed (e.g., on input)
    UpdateView();
    UpdateProjection();
}

void PerspectiveCamera::ProcessKeyboard(float deltaTime, bool forward, bool backward, bool left, bool right)
{
    float velocity = m_MovementSpeed * deltaTime;
    if (forward)
        m_Position += m_Front * velocity;
    if (backward)
        m_Position -= m_Front * velocity;
    if (left)
        m_Position -= m_Right * velocity;
    if (right)
        m_Position += m_Right * velocity;

    UpdateView();
}

void PerspectiveCamera::ProcessMouseMovement(float xoffset, float yoffset)
{
    xoffset *= m_MouseSensitivity;
    yoffset *= m_MouseSensitivity;

    m_Yaw += xoffset;
    m_Pitch += yoffset;

    // Clamp pitch
    m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

    // Recalculate front, right, up
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));

    UpdateView();
}

void PerspectiveCamera::SetAspectRatio(float aspect)
{
    m_Aspect = aspect;
    UpdateProjection();
}

void PerspectiveCamera::SetFOV(float fov)
{
    m_FOV = std::clamp(fov, 1.0f, 120.0f);
    UpdateProjection();
}

void PerspectiveCamera::SetClipPlanes(float nearPlane, float farPlane)
{
    m_Near = std::max(nearPlane, 0.001f);
    m_Far = std::max(farPlane, m_Near + 0.001f);
    UpdateProjection();
}

void PerspectiveCamera::UpdateView()
{
    m_View = glm::lookAt(m_Position, m_Position + m_Front, m_WorldUp);
}

void PerspectiveCamera::UpdateProjection()
{
    m_Projection = glm::perspective(glm::radians(m_FOV), m_Aspect, m_Near, m_Far);
}