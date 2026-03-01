#pragma once
#include "PerspectiveCamera.h"
#include "Input.h"

class EditorCamera : public PerspectiveCamera
{
public:
    EditorCamera(float fov, float aspect, float nearClip, float farClip)
        : PerspectiveCamera(fov, aspect, nearClip, farClip)
    {
    }

    void Update(float dt) override
    {
        const float speed = 5.0f;
        const float sensitivity = 0.002f;

        if (Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
        {
            m_Yaw += Input::GetMouseDeltaX() * sensitivity;
            m_Pitch -= Input::GetMouseDeltaY() * sensitivity;

            m_Pitch = glm::clamp(m_Pitch, -1.5f, 1.5f);

            glm::vec3 forward = {
                cos(m_Yaw) * cos(m_Pitch),
                sin(m_Pitch),
                sin(m_Yaw) * cos(m_Pitch)};

            glm::vec3 right = glm::normalize(glm::cross(forward, {0, 1, 0}));

            if (Input::IsKeyPressed(GLFW_KEY_W))
                m_Position += forward * speed * dt;
            if (Input::IsKeyPressed(GLFW_KEY_S))
                m_Position -= forward * speed * dt;
            if (Input::IsKeyPressed(GLFW_KEY_A))
                m_Position -= right * speed * dt;
            if (Input::IsKeyPressed(GLFW_KEY_D))
                m_Position += right * speed * dt;

            m_View = glm::lookAt(m_Position, m_Position + forward, {0, 1, 0});
        }
    }

private:
    glm::vec3 m_Position = {0.0f, 0.0f, 3.0f};
    float m_Yaw = -glm::half_pi<float>();
    float m_Pitch = 0.0f;
};