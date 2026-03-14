#include "Model.h"
#include "AnimationComponent.h"

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
    {
        mesh.Draw(shader);
    }
}

void Model::loadModel(const std::string &path)
{
    m_Scene = m_Importer.ReadFile(
        path,
        aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices |
            aiProcess_LimitBoneWeights);

    if (!m_Scene || m_Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_Scene->mRootNode)
    {
        std::cout << "ASSIMP ERROR: " << m_Importer.GetErrorString() << std::endl;
        return;
    }

    // Use std::filesystem for proper path handling across platforms
    std::filesystem::path modelPath(path);
    directory = modelPath.parent_path().string();

    std::cout << "Model loaded from: " << path << std::endl;
    std::cout << "Directory extracted: " << directory << std::endl;

    if (m_Scene->HasAnimations())
    {
        std::cout << "Model has " << m_Scene->mNumAnimations << " animations" << std::endl;
    }

    // Debug: Print skeleton hierarchy
    std::cout << "[Model] Skeleton hierarchy:" << std::endl;
    PrintNodeHierarchy(m_Scene->mRootNode, 0);

    processNode(m_Scene->mRootNode, m_Scene);
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

    // Extract bone weights if the mesh has bones
    if (mesh->HasBones())
    {
        ExtractBoneWeights(vertices, mesh, m_BoneInfoMap, m_BoneCount);

        // Normalize bone weights so they sum to 1.0 per vertex
        for (auto &vertex : vertices)
        {
            float totalWeight = vertex.BoneWeights.x + vertex.BoneWeights.y +
                                vertex.BoneWeights.z + vertex.BoneWeights.w;

            if (totalWeight > 0.0f)
            {
                vertex.BoneWeights /= totalWeight;
            }
        }
    }

    // Defer OpenGL object creation to main thread
    return Mesh(vertices, indices, textures, true); // deferSetup = true
}

std::vector<Texture> Model::loadMaterialTextures(
    aiMaterial *mat,
    aiTextureType type,
    const std::string &typeName)
{
    std::vector<Texture> textures;

    // Note: When models are loaded from worker threads (async loading),
    // texture creation is skipped because OpenGL operations must happen on the main thread.
    // Textures can be loaded later or the model will render with default white color.

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        std::string filename = std::string(str.C_Str());

        // Used std::filesystem for proper path construction
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

            // Async-safe path: keep texture metadata only.
            // GPU upload is deferred to Mesh::Draw on the render thread.
            Texture texture;
            texture.ID = 0;
            texture.typeName = typeName;
            texture.path = fullPath;
            texture.type = GL_TEXTURE_2D;
            textures.push_back(texture);
        }
    }

    return textures;
}

void Model::ExtractBoneWeights(std::vector<Vertex> &vertices, aiMesh *mesh,
                               std::map<std::string, MaraGl::BoneInfo> &boneInfoMap,
                               int &boneCount)
{
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (boneInfoMap.find(boneName) == boneInfoMap.end())
        {
            MaraGl::BoneInfo newBoneInfo;
            newBoneInfo.id = boneCount;

            // Convert Assimp matrix to GLM (must match ConvertMatrixToGLM in Animator)
            aiMatrix4x4 offsetMatrix = mesh->mBones[boneIndex]->mOffsetMatrix;
            newBoneInfo.offset[0][0] = offsetMatrix.a1;
            newBoneInfo.offset[1][0] = offsetMatrix.a2;
            newBoneInfo.offset[2][0] = offsetMatrix.a3;
            newBoneInfo.offset[3][0] = offsetMatrix.a4;
            newBoneInfo.offset[0][1] = offsetMatrix.b1;
            newBoneInfo.offset[1][1] = offsetMatrix.b2;
            newBoneInfo.offset[2][1] = offsetMatrix.b3;
            newBoneInfo.offset[3][1] = offsetMatrix.b4;
            newBoneInfo.offset[0][2] = offsetMatrix.c1;
            newBoneInfo.offset[1][2] = offsetMatrix.c2;
            newBoneInfo.offset[2][2] = offsetMatrix.c3;
            newBoneInfo.offset[3][2] = offsetMatrix.c4;
            newBoneInfo.offset[0][3] = offsetMatrix.d1;
            newBoneInfo.offset[1][3] = offsetMatrix.d2;
            newBoneInfo.offset[2][3] = offsetMatrix.d3;
            newBoneInfo.offset[3][3] = offsetMatrix.d4;

            boneInfoMap[boneName] = newBoneInfo;
            boneID = boneCount;
            boneCount++;
        }
        else
        {
            boneID = boneInfoMap[boneName].id;
        }

        assert(boneID != -1);

        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= vertices.size());
            SetVertexBoneData(vertices[vertexId], boneID, weight);
        }
    }

    std::cout << "[ExtractBoneWeights] Processed " << mesh->mNumBones
              << " bones, total bone count: " << boneCount << std::endl;
}

void Model::SetVertexBoneData(Vertex &vertex, int boneID, float weight)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (vertex.BoneIDs[i] < 0)
        {
            vertex.BoneWeights[i] = weight;
            vertex.BoneIDs[i] = boneID;
            break;
        }
    }
}
std::vector<MaraGl::Animation> Model::LoadAnimations()
{
    std::vector<MaraGl::Animation> animations;

    if (!m_Scene || !m_Scene->HasAnimations())
    {
        std::cout << "[LoadAnimations] No animations in scene" << std::endl;
        return animations;
    }

    for (unsigned int i = 0; i < m_Scene->mNumAnimations; i++)
    {
        aiAnimation *anim = m_Scene->mAnimations[i];
        MaraGl::Animation animation;
        animation.name = anim->mName.C_Str();
        animation.duration = anim->mDuration;
        animation.ticksPerSecond = anim->mTicksPerSecond > 0 ? anim->mTicksPerSecond : 30.0f;

        // Debug: Show animation channels
        std::cout << "[LoadAnimations] Animation '" << animation.name
                  << "' has " << anim->mNumChannels << " channels:" << std::endl;
        // for (unsigned int c = 0; c < anim->mNumChannels; c++)
        // {
        //     std::cout << "  - " << anim->mChannels[c]->mNodeName.C_Str() << std::endl;
        // }

        // Load animation channels
        for (unsigned int channelIndex = 0; channelIndex < anim->mNumChannels; channelIndex++)
        {
            aiNodeAnim *channel = anim->mChannels[channelIndex];
            std::string boneName = channel->mNodeName.C_Str();

            // Strip Assimp FBX suffixes (e.g., "_$AssimpFbx$_Rotation")
            // These are added by Assimp for FBX files but don't match actual bone names
            size_t pos = boneName.find("_$AssimpFbx$_");
            if (pos != std::string::npos)
            {
                boneName = boneName.substr(0, pos);
            }

            // Check if this bone is in our boneInfoMap
            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
            {
                std::cout << "  WARNING: Animation channel for bone '" << boneName
                          << "' not found in bone map!" << std::endl;
            }

            MaraGl::BoneAnimation boneAnim;
            boneAnim.boneName = boneName;

            // Load position keys
            for (unsigned int p = 0; p < channel->mNumPositionKeys; p++)
            {
                aiVectorKey posKey = channel->mPositionKeys[p];
                MaraGl::PositionKey key;
                key.position = glm::vec3(posKey.mValue.x, posKey.mValue.y, posKey.mValue.z);
                key.timeStamp = (float)posKey.mTime;
                boneAnim.positions.push_back(key);
            }

            // Load rotation keys
            for (unsigned int r = 0; r < channel->mNumRotationKeys; r++)
            {
                aiQuatKey rotKey = channel->mRotationKeys[r];
                MaraGl::RotationKey key;
                key.orientation = glm::quat(rotKey.mValue.w, rotKey.mValue.x, rotKey.mValue.y, rotKey.mValue.z);
                key.timeStamp = (float)rotKey.mTime;
                boneAnim.rotations.push_back(key);
            }

            // Load scale keys
            for (unsigned int s = 0; s < channel->mNumScalingKeys; s++)
            {
                aiVectorKey scaleKey = channel->mScalingKeys[s];
                MaraGl::ScaleKey key;
                key.scale = glm::vec3(scaleKey.mValue.x, scaleKey.mValue.y, scaleKey.mValue.z);
                key.timeStamp = (float)scaleKey.mTime;
                boneAnim.scales.push_back(key);
            }

            animation.boneAnimations.push_back(boneAnim);
        }

        animations.push_back(animation);

        std::cout << "[LoadAnimations] Loaded '" << animation.name
                  << "' - Duration: " << animation.duration
                  << ", TPS: " << animation.ticksPerSecond
                  << ", Channels: " << animation.boneAnimations.size() << std::endl;
    }

    std::cout << "[LoadAnimations] Total animations loaded: " << animations.size() << std::endl;

    // Debug: Show all bones in boneInfoMap
    std::cout << "[LoadAnimations] Total bones in skeleton: " << m_BoneInfoMap.size() << std::endl;
    std::cout << "[LoadAnimations] Bones in skeleton:" << std::endl;
    for (const auto &[boneName, boneInfo] : m_BoneInfoMap)
    {
        std::cout << "  - " << boneName << " (ID: " << boneInfo.id << ")" << std::endl;
    }

    return animations;
}

void Model::PrintNodeHierarchy(aiNode *node, int depth) const
{
    if (!node)
        return;

    std::string indent(depth * 2, ' ');
    std::string nodeName = node->mName.C_Str();

    // Check if this node is a bone
    auto it = m_BoneInfoMap.find(nodeName);
    std::string boneInfo = (it != m_BoneInfoMap.end()) ? " [BONE ID: " + std::to_string(it->second.id) + "]" : "";

    std::cout << indent << "- " << nodeName << boneInfo << std::endl;

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        PrintNodeHierarchy(node->mChildren[i], depth + 1);
    }
}
