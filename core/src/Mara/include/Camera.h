#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
    virtual ~Camera() = default;

    virtual void Update(float deltaTime) = 0;

    const glm::mat4 &GetView() const { return m_View; }
    const glm::mat4 &GetProjection() const { return m_Projection; }

protected:
    glm::mat4 m_View = glm::mat4(1.0f);
    glm::mat4 m_Projection = glm::mat4(1.0f);
};