#pragma once
#include <glm/glm.hpp>

struct Transform
{
    glm::vec3 Position{0.0f};
    glm::vec3 Rotation{0.0f};
    glm::vec3 Scale{1.0f};

    glm::mat4 GetMatrix() const;
};