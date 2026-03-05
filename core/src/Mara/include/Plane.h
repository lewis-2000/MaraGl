#pragma once

#include <memory>

class Shader;

class Plane
{
public:
    Plane(float width = 10.0f, float height = 10.0f);
    ~Plane();

    void Draw(Shader &shader);

private:
    void CreatePlaneGeometry(float width, float height);

    unsigned int VAO, VBO, EBO;
    unsigned int indexCount;
};
