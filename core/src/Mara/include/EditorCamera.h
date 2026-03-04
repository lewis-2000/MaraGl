#pragma once
#include "PerspectiveCamera.h"
#include "Input.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <GLFW/glfw3.h>

namespace MaraGl
{
    class EditorCamera : public PerspectiveCamera
    {
    public:
        EditorCamera(float fov, float aspect, float nearClip, float farClip)
            : PerspectiveCamera(fov, aspect, nearClip, farClip)
        {
        }

        void Update(float dt) override
        {
            Update(dt, true); // Default: always process input
        }

        void Update(float dt, bool processInput)
        {
            const float speed = 5.0f;
            const float sensitivity = 0.01f; // Increased from 0.002f for snappier response

            // Calculate forward and right vectors for movement
            glm::vec3 forward = {
                cos(m_Yaw) * cos(m_Pitch),
                sin(m_Pitch),
                sin(m_Yaw) * cos(m_Pitch)};

            glm::vec3 right = glm::normalize(glm::cross(forward, {0, 1, 0}));

            // Only process input if allowed (e.g., when ScenePanel is focused)
            if (processInput)
            {
                // Keyboard movement (works anytime)
                if (Input::IsKeyPressed(GLFW_KEY_W))
                    m_Position += forward * speed * dt;
                if (Input::IsKeyPressed(GLFW_KEY_S))
                    m_Position -= forward * speed * dt;
                if (Input::IsKeyPressed(GLFW_KEY_A))
                    m_Position -= right * speed * dt;
                if (Input::IsKeyPressed(GLFW_KEY_D))
                    m_Position += right * speed * dt;

                // Vertical movement (Y-axis)
                if (Input::IsKeyPressed(GLFW_KEY_E) || Input::IsKeyPressed(GLFW_KEY_SPACE))
                    m_Position.y += speed * dt; // Move up
                if (Input::IsKeyPressed(GLFW_KEY_Q) || Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
                    m_Position.y -= speed * dt; // Move down

                // Mouse rotation (only when right-click is held)
                if (Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
                {
                    float deltaX = Input::GetMouseDeltaX();
                    float deltaY = Input::GetMouseDeltaY();

                    m_Yaw += deltaX * sensitivity;
                    m_Pitch -= deltaY * sensitivity;
                    m_Pitch = glm::clamp(m_Pitch, -1.5f, 1.5f);
                }
            }

            // Always update view matrix
            m_View = glm::lookAt(m_Position, m_Position + forward, {0, 1, 0});
        }

    private:
        glm::vec3 m_Position = {0.0f, 0.0f, 3.0f};
        float m_Yaw = -glm::half_pi<float>();
        float m_Pitch = 0.0f;
    };
} // namespace MaraGl