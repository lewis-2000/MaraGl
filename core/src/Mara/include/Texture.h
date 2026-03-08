#pragma once

#include <glad/glad.h>
#include <stb_image.h>
#include <string>

#include "Shader.h"

class Texture
{
public:
    GLuint ID;
    std::string typeName;
    std::string path;
    GLenum type;

    Texture() : ID(0), type(GL_TEXTURE_2D) {} // Default constructor

    Texture(const char *image,
            GLenum texType,
            GLenum slot,
            GLenum format,
            GLenum pixelType,
            const std::string &texName);

    void texUnit(Shader &shader, const char *uniform, GLuint unit);
    void Bind();
    void Unbind();
    void Delete();
};