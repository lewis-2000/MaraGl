#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Transform::GetMatrix() const
{
    glm::mat4 mat(1.0f);
    mat = glm::translate(mat, Position);
    mat = glm::rotate(mat, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
    mat = glm::rotate(mat, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
    mat = glm::rotate(mat, glm::radians(Rotation.z), glm::vec3(0, 0, 1));
    mat = glm::scale(mat, Scale);
    return mat;
}