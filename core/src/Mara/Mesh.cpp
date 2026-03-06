#include "Mesh.h"
#include <string>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures)
    : vertices(vertices), indices(indices), textures(textures)
{
    setupMesh();
}

void Mesh::setupMesh()
{
    vao.Bind();

    // Create VBO & EBO
    vbo = new VBO((GLfloat *)vertices.data(), vertices.size() * sizeof(Vertex));
    ebo = new EBO(indices.data(), indices.size() * sizeof(GLuint));

    // Vertex attributes
    vao.LinkAttrib(*vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void *)0);                           // Position
    vao.LinkAttrib(*vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void *)offsetof(Vertex, Normal));    // Normal
    vao.LinkAttrib(*vbo, 2, 2, GL_FLOAT, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords)); // TexCoords

    // Bone data
    vao.LinkAttrib(*vbo, 3, 4, GL_INT, sizeof(Vertex), (void *)offsetof(Vertex, BoneIDs));       // Bone IDs
    vao.LinkAttrib(*vbo, 4, 4, GL_FLOAT, sizeof(Vertex), (void *)offsetof(Vertex, BoneWeights)); // Bone Weights

    vao.Unbind();
    vbo->Unbind();
    ebo->Unbind();
}

void Mesh::Draw(Shader &shader)
{
    shader.use();

    if (textures.empty())
    {
        // Create a 1x1 white texture on first use
        static GLuint defaultWhiteTex = 0;
        if (defaultWhiteTex == 0)
        {
            unsigned char whitePixel[3] = {255, 255, 255};
            glGenTextures(1, &defaultWhiteTex);
            glBindTexture(GL_TEXTURE_2D, defaultWhiteTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, defaultWhiteTex);
        shader.setInt("texture_diffuse1", 0);
    }
    else
    {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        for (unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // Activate texture unit

            std::string number;
            std::string name = textures[i].typeName;

            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "texture_normal")
                number = std::to_string(normalNr++);
            else if (name == "texture_height")
                number = std::to_string(heightNr++);

            shader.setInt((name + number).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].ID);
        }
    }

    // Draw mesh
    vao.Bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    vao.Unbind();

    glActiveTexture(GL_TEXTURE0); // Reset
}
