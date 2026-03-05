#include "Plane.h"
#include "Shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>

Plane::Plane(float width, float height)
    : VAO(0), VBO(0), EBO(0), indexCount(0)
{
    CreatePlaneGeometry(width, height);
}

Plane::~Plane()
{
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
    if (EBO)
        glDeleteBuffers(1, &EBO);
}

void Plane::CreatePlaneGeometry(float width, float height)
{
    // Vertices: position (3), normal (3), texcoord (2)
    float vertices[] = {
        // positions           // normals           // texture coords
        -width / 2,
        0.0f,
        -height / 2,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        width / 2,
        0.0f,
        -height / 2,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        width / 2,
        0.0f,
        height / 2,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        -width / 2,
        0.0f,
        height / 2,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
    };

    unsigned int indices[] = {
        0,
        1,
        2,
        2,
        3,
        0,
    };

    indexCount = 6;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Plane::Draw(Shader &shader)
{
    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
