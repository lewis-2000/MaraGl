#pragma once

class Shader;

class LightGizmo
{
public:
    LightGizmo(float radius = 0.3f, int segments = 16);
    ~LightGizmo();

    void Draw(Shader &shader);

private:
    void CreateSphereGeometry(float radius, int segments);
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    int indexCount = 0;
};
