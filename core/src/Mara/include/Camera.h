#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    virtual ~Camera() = default;

    // Update camera every frame (e.g., movement, input)
    virtual void Update(float deltaTime) = 0;

    // Access matrices
    const glm::mat4 &GetView() const { return m_View; }
    const glm::mat4 &GetProjection() const { return m_Projection; }

    // Position & rotation
    const glm::vec3 &GetPosition() const { return m_Position; }
    void SetPosition(const glm::vec3 &pos)
    {
        m_Position = pos;
        UpdateView();
    }

    const glm::vec3 &GetFront() const { return m_Front; }
    const glm::vec3 &GetUp() const { return m_Up; }
    const glm::vec3 &GetRight() const { return m_Right; }

protected:
    // Recalculate view matrix
    void UpdateView()
    {
        m_View = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    }

protected:
    // Camera transform
    glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 m_Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_Right = glm::cross(m_Front, m_Up);

    glm::mat4 m_View = glm::mat4(1.0f);
    glm::mat4 m_Projection = glm::mat4(1.0f);

    float m_Yaw = -90.0f; // Rotation around Y axis
    float m_Pitch = 0.0f; // Rotation around X axis
};