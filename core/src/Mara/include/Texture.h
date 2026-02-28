#pragma once

#include <glad/glad.h>
#include <stb_image.h>
#include <string>

#include "Shader.h"

class Texture {
public:
    GLuint ID;           // OpenGL texture ID
    GLenum type;         // OpenGL texture target (e.g. GL_TEXTURE_2D)
    std::string typeName; // "texture_diffuse", "texture_specular", etc.
    std::string path;    // Path to image file (used to avoid duplicate loads)

    Texture(const char* image, GLenum texType, GLenum slot, GLenum format, GLenum pixelType);

    // Assigns a texture unit to a texture
    void texUnit(Shader& shader, const char* uniform, GLuint unit);

    // Binds a texture
    void Bind();
    // Unbinds a texture
    void Unbind();
    // Deletes a texture
    void Delete();
};
