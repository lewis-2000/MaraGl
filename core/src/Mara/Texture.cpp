#include "include/Texture.h"
#include <iostream>

Texture::Texture(const char *image, GLenum texType, GLenum slot, GLenum format, GLenum pixelType, const std::string &texName)
    : type(texType), typeName(texName)
{
    // Generate texture ID
    glGenTextures(1, &ID);
    glActiveTexture(slot);
    glBindTexture(texType, ID);

    // Texture parameters
    glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load image with stb_image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(image, &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;

        if (nrChannels == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrChannels == 3)
        {
            internalFormat = GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrChannels == 4)
        {
            internalFormat = GL_RGBA;
            dataFormat = GL_RGBA;
        }
        else
        {
            std::cerr << "Unsupported channel count: " << nrChannels << " in " << image << std::endl;
            stbi_image_free(data);
            return;
        }

        glTexImage2D(texType, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(texType);
        path = std::string(image);

        // Log texture loading
        std::cout << "Loading " << image << " ("
                  << width << "x" << height << ", channels=" << nrChannels << ")\n";
    }

    stbi_image_free(data);
    glBindTexture(texType, 0);
}

void Texture::texUnit(Shader &shader, const char *uniform, GLuint unit)
{
    GLuint texUniform = glGetUniformLocation(shader.ID, uniform);
    shader.use();
    glUniform1i(texUniform, unit);
}

void Texture::Bind()
{
    glBindTexture(type, ID);
}

void Texture::Unbind()
{
    glBindTexture(type, 0);
}

void Texture::Delete()
{
    glDeleteTextures(1, &ID);
}
