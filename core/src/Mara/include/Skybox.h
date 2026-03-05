#pragma once

#include <string>

class Shader;

class Skybox
{
public:
    Skybox();
    ~Skybox();

    void Draw(Shader &shader, bool useEquirectangular = true);

    // Load cubemap from 6 individual images
    bool LoadCubemap(const std::string &right, const std::string &left,
                     const std::string &top, const std::string &bottom,
                     const std::string &front, const std::string &back);

    // Load equirectangular HDRI texture (simpler format)
    bool LoadEquirectangular(const std::string &hdrPath);

    unsigned int GetCubemapTexture() const { return cubemapTexture; }
    unsigned int GetEquirectTexture() const { return equirectTexture; }
    bool IsEquirectangular() const { return equirectTexture != 0; }

private:
    void CreateSkyboxGeometry();
    unsigned int cubemapTexture = 0;
    unsigned int equirectTexture = 0;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
};
