#include "LightGizmo.h"
#include "Shader.h"
#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

LightGizmo::LightGizmo(float radius, int segments)
{
    CreateSphereGeometry(radius, segments);
}

LightGizmo::~LightGizmo()
{
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
    if (EBO)
        glDeleteBuffers(1, &EBO);
}

void LightGizmo::CreateSphereGeometry(float radius, int segments)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Generate sphere vertices using spherical coordinates
    for (int lat = 0; lat <= segments; ++lat)
    {
        float theta = lat * glm::pi<float>() / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= segments; ++lon)
        {
            float phi = lon * 2.0f * glm::pi<float>() / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
        }
    }

    // Generate indices for triangle strips
    for (int lat = 0; lat < segments; ++lat)
    {
        for (int lon = 0; lon < segments; ++lon)
        {
            int first = lat * (segments + 1) + lon;
            int second = first + segments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    indexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    glBindVertexArray(0);
}

void LightGizmo::Draw(Shader &shader)
{
    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
