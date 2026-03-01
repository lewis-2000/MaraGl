#pragma once
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

class PerspectiveCamera : public Camera
{
public:
    PerspectiveCamera(float fov, float aspect, float nearClip, float farClip)
        : m_FOV(fov), m_Aspect(aspect),
          m_Near(nearClip), m_Far(farClip)
    {
        RecalculateProjection();
    }

    void SetViewportSize(float width, float height)
    {
        m_Aspect = width / height;
        RecalculateProjection();
    }

protected:
    void RecalculateProjection()
    {
        m_Projection = glm::perspective(
            glm::radians(m_FOV),
            m_Aspect,
            m_Near,
            m_Far);
    }

protected:
    float m_FOV;
    float m_Aspect;
    float m_Near;
    float m_Far;
};