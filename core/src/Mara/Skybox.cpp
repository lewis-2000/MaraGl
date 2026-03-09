#include "Skybox.h"
#include "Shader.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <utility>
#include <zlib.h>

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 0
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

Skybox::Skybox()
{
    CreateSkyboxGeometry();
}

Skybox::~Skybox()
{
    if (VAO)
        glDeleteVertexArrays(1, &VAO);
    if (VBO)
        glDeleteBuffers(1, &VBO);
    if (cubemapTexture)
        glDeleteTextures(1, &cubemapTexture);
    if (equirectTexture)
        glDeleteTextures(1, &equirectTexture);
}

void Skybox::CreateSkyboxGeometry()
{
    // Skybox cube vertices (position only)
    float skyboxVertices[] = {
        // positions
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f};

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
}

void Skybox::Draw(Shader &shader, bool useEquirectangular)
{
    shader.use();

    static bool s_LoggedMissingTexture = false;

    if (cubemapTexture && !useEquirectangular)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        shader.setInt("skybox", 0);
    }
    else if (equirectTexture && useEquirectangular)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, equirectTexture);
        shader.setInt("equirectangularMap", 0);
    }
    else if (!s_LoggedMissingTexture)
    {
        std::cerr << "[Skybox] Draw called with no matching texture bound."
                  << " useEquirectangular=" << useEquirectangular
                  << " equirectTex=" << equirectTexture
                  << " cubemapTex=" << cubemapTexture
                  << std::endl;
        s_LoggedMissingTexture = true;
    }

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        std::cerr << "[Skybox] OpenGL error after draw: 0x" << std::hex << err << std::dec << std::endl;
    }
}

bool Skybox::LoadCubemap(const std::string &right, const std::string &left,
                         const std::string &top, const std::string &bottom,
                         const std::string &front, const std::string &back)
{
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    const std::string paths[] = {right, left, top, bottom, front, back};
    const GLenum targets[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};

    for (int i = 0; i < 6; i++)
    {
        int width, height, nrChannels;
        unsigned char *data = stbi_load(paths[i].c_str(), &width, &height, &nrChannels, 0);

        if (data)
        {
            glTexImage2D(targets[i], 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cerr << "Failed to load cubemap texture: " << paths[i] << std::endl;
            return false;
        }
    }

    return true;
}

bool Skybox::LoadEquirectangular(const std::string &hdrPath)
{
    const std::filesystem::path inPath(hdrPath);
    const std::string ext = inPath.has_extension() ? inPath.extension().string() : std::string();

    int width = 0;
    int height = 0;
    int nrComponents = 0;
    float *data = nullptr;
    bool loadedWithTinyExr = false;

    if (ext == ".exr" || ext == ".EXR")
    {
        const char *err = nullptr;
        int ret = LoadEXR(&data, &width, &height, hdrPath.c_str(), &err);
        if (ret != TINYEXR_SUCCESS || !data)
        {
            std::cerr << "Failed to load equirectangular EXR image: " << hdrPath
                      << " reason='" << (err ? err : "unknown") << "'"
                      << std::endl;
            if (err)
                FreeEXRErrorMessage(err);
            return false;
        }

        // TinyEXR returns RGBA float data; flip vertically to match stb_image path.
        nrComponents = 4;
        for (int y = 0; y < height / 2; ++y)
        {
            const int top = y * width * nrComponents;
            const int bottom = (height - 1 - y) * width * nrComponents;
            for (int x = 0; x < width * nrComponents; ++x)
            {
                std::swap(data[top + x], data[bottom + x]);
            }
        }

        loadedWithTinyExr = true;
    }
    else
    {
        stbi_set_flip_vertically_on_load(true);
        data = stbi_loadf(hdrPath.c_str(), &width, &height, &nrComponents, 0);
        stbi_set_flip_vertically_on_load(false);

        if (!data)
        {
            const char *reason = stbi_failure_reason();
            std::cerr << "Failed to load equirectangular image: " << hdrPath
                      << " reason='" << (reason ? reason : "unknown") << "'"
                      << std::endl;
            return false;
        }
    }

    // Create equirectangular texture (2D, not cubemap)
    GLenum pixelFormat = (nrComponents == 4) ? GL_RGBA : GL_RGB;
    GLenum internalFormat = (nrComponents == 4) ? GL_RGBA16F : GL_RGB16F;

    glGenTextures(1, &equirectTexture);
    glBindTexture(GL_TEXTURE_2D, equirectTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, pixelFormat, GL_FLOAT, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (loadedWithTinyExr)
        free(data);
    else
        stbi_image_free(data);

    std::cout << "Loaded equirectangular skybox: " << hdrPath
              << " (" << width << "x" << height << ")"
              << " components=" << nrComponents
              << " texId=" << equirectTexture
              << " loader=" << (loadedWithTinyExr ? "tinyexr" : "stb")
              << std::endl;

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        std::cerr << "[Skybox] OpenGL error after texture upload: 0x" << std::hex << err << std::dec << std::endl;
    }

    return true;
}
