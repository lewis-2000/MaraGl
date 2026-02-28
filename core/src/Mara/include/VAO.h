#pragma once

#include <glad/glad.h>
#include "VBO.h"

class VAO
{
public:
    GLuint ID;
    VAO();

    // Flexible attribute linker
    void LinkAttrib(
        VBO& vbo,
        GLuint layout,            // layout index (shader)
        GLint numComponents,      // e.g. 3 for vec3, 2 for vec2
        GLenum type,              // e.g. GL_FLOAT
        GLsizei stride,           // size of entire vertex (in bytes)
        void* offset              // offset in vertex struct
    );

    void Bind();
    void Unbind();
    void Delete();
};

