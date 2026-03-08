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

        const glm::vec3 &GetEditorPosition() const { return m_Position; }
        float GetYaw() const { return m_Yaw; }
        float GetPitch() const { return m_Pitch; }
        float GetMoveSpeed() const { return m_MoveSpeed; }
        float GetMouseSensitivity() const { return m_MouseSensitivity; }

        void SetEditorPosition(const glm::vec3 &position)
        {
            m_Position = position;
            RebuildView();
        }

        void SetYawPitch(float yaw, float pitch)
        {
            m_Yaw = yaw;
            m_Pitch = glm::clamp(pitch, -1.5f, 1.5f);
            RebuildView();
        }

        void SetMoveSpeed(float speed)
        {
            m_MoveSpeed = glm::max(speed, 0.01f);
        }

        void SetMouseSensitivity(float sensitivity)
        {
            m_MouseSensitivity = glm::max(sensitivity, 0.0001f);
        }

        void Update(float dt, bool processInput)
        {
            // Calculate forward and right vectors for movement
            glm::vec3 forward = ComputeForward();
            glm::vec3 right = glm::normalize(glm::cross(forward, {0, 1, 0}));

            // Only process input if allowed (e.g., when ScenePanel is focused)
            if (processInput)
            {
                // Keyboard movement (works anytime)
                if (Input::IsKeyPressed(GLFW_KEY_W))
                    m_Position += forward * m_MoveSpeed * dt;
                if (Input::IsKeyPressed(GLFW_KEY_S))
                    m_Position -= forward * m_MoveSpeed * dt;
                if (Input::IsKeyPressed(GLFW_KEY_A))
                    m_Position -= right * m_MoveSpeed * dt;
                if (Input::IsKeyPressed(GLFW_KEY_D))
                    m_Position += right * m_MoveSpeed * dt;

                // Vertical movement (Y-axis)
                if (Input::IsKeyPressed(GLFW_KEY_E) || Input::IsKeyPressed(GLFW_KEY_SPACE))
                    m_Position.y += m_MoveSpeed * dt; // Move up
                if (Input::IsKeyPressed(GLFW_KEY_Q) || Input::IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
                    m_Position.y -= m_MoveSpeed * dt; // Move down

                // Mouse rotation (only when right-click is held)
                if (Input::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
                {
                    float deltaX = Input::GetMouseDeltaX();
                    float deltaY = Input::GetMouseDeltaY();

                    m_Yaw += deltaX * m_MouseSensitivity;
                    m_Pitch -= deltaY * m_MouseSensitivity;
                    m_Pitch = glm::clamp(m_Pitch, -1.5f, 1.5f);
                }
            }

            // Always update view matrix
            RebuildView();
        }

    private:
        glm::vec3 ComputeForward() const
        {
            return glm::normalize(glm::vec3(
                cos(m_Yaw) * cos(m_Pitch),
                sin(m_Pitch),
                sin(m_Yaw) * cos(m_Pitch)));
        }

        void RebuildView()
        {
            glm::vec3 forward = ComputeForward();
            m_View = glm::lookAt(m_Position, m_Position + forward, {0, 1, 0});
        }

        glm::vec3 m_Position = {0.0f, 0.0f, 3.0f};
        float m_Yaw = -glm::half_pi<float>();
        float m_Pitch = 0.0f;
        float m_MoveSpeed = 5.0f;
        float m_MouseSensitivity = 0.01f;
    };
} // namespace MaraGl