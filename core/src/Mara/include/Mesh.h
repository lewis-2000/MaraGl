#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"
#include "Shader.h"

// Each vertex contains position, normal, texCoords, bone data
struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::ivec4 BoneIDs = glm::ivec4(-1);     // Up to 4 bones per vertex
    glm::vec4 BoneWeights = glm::vec4(0.0f); // Weights for each bone
};

#define MAX_BONE_INFLUENCE 4

class Mesh
{
public:
    // Mesh data
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    // OpenGL objects
    VAO vao;
    VBO *vbo;
    EBO *ebo;

    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures, bool deferSetup = false);

    void Draw(Shader &shader);
    void InitializeGLObjects(); // Call this on main thread after creation
    static void ClearTextureCache();
    bool IsInitialized() const { return m_GLObjectsInitialized; }

private:
    void setupMesh();
    bool m_GLObjectsInitialized = false;
};
