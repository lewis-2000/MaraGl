#include "Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Model::Model(const std::string &path)
{
    loadModel(path);
}

void Model::Draw(Shader &shader)
{
    for (auto &mesh : meshes)
        mesh.Draw(shader);
}

void Model::loadModel(const std::string &path)
{
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ASSIMP ERROR: " << importer.GetErrorString() << std::endl;
        return;
    }

    // Use std::filesystem for proper path handling across platforms
    std::filesystem::path modelPath(path);
    directory = modelPath.parent_path().string();

    std::cout << "Model loaded from: " << path << std::endl;
    std::cout << "Directory extracted: " << directory << std::endl;

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
        processNode(node->mChildren[i], scene);
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        vertex.Position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z};

        // std::cout << "Vertex Position: " << vertex.Position.x << ", "
        //           << vertex.Position.y << ", "
        //           << vertex.Position.z << std::endl;

        vertex.Normal = mesh->HasNormals()
                            ? glm::vec3(
                                  mesh->mNormals[i].x,
                                  mesh->mNormals[i].y,
                                  mesh->mNormals[i].z)
                            : glm::vec3(0.0f);

        if (mesh->mTextureCoords[0])
        {
            vertex.TexCoords = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y};
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        auto diffuseMaps = loadMaterialTextures(
            material,
            aiTextureType_DIFFUSE,
            "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        auto specularMaps = loadMaterialTextures(
            material,
            aiTextureType_SPECULAR,
            "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(
    aiMaterial *mat,
    aiTextureType type,
    const std::string &typeName)
{
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        std::string filename = std::string(str.C_Str());

        // Use std::filesystem for proper path construction
        std::filesystem::path texturePath = std::filesystem::path(directory) / filename;
        std::string fullPath = texturePath.string();

        bool skip = false;
        for (auto &loaded : texturesLoaded)
        {
            if (loaded.path == fullPath)
            {
                textures.push_back(loaded);
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            // Check if file exists before trying to load
            if (!std::filesystem::exists(fullPath))
            {
                std::cout << "Texture file not found: " << fullPath << std::endl;
                continue;
            }

            Texture texture(
                fullPath.c_str(),
                GL_TEXTURE_2D,
                GL_TEXTURE0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                typeName);
            glGenTextures(1, &texture.ID);

            int width, height, nrChannels;
            unsigned char *data = stbi_load(fullPath.c_str(),
                                            &width, &height,
                                            &nrChannels, 0);

            if (data)
            {
                std::cout << "Successfully loaded texture: " << fullPath << std::endl;

                GLenum format = GL_RGB;
                if (nrChannels == 1)
                    format = GL_RED;
                else if (nrChannels == 3)
                    format = GL_RGB;
                else if (nrChannels == 4)
                    format = GL_RGBA;

                glBindTexture(GL_TEXTURE_2D, texture.ID);
                glTexImage2D(GL_TEXTURE_2D, 0, format,
                             width, height, 0,
                             format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                stbi_image_free(data);

                texture.typeName = typeName;
                texture.path = fullPath;

                textures.push_back(texture);
                texturesLoaded.push_back(texture);
            }
            else
            {
                std::cout << "Failed to load texture data from: " << fullPath << std::endl;
                stbi_image_free(data);
            }
        }
    }

    return textures;
}
