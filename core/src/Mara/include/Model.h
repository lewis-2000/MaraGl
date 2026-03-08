#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "Mesh.h"
#include "AnimationComponent.h"

class Model
{
public:
    Model(const std::string &path);
    void Draw(Shader &shader);

    // Animation support
    const aiScene *GetScene() const { return m_Scene; }
    bool HasAnimations() const { return m_Scene && m_Scene->mNumAnimations > 0; }
    unsigned int GetAnimationCount() const { return m_Scene ? m_Scene->mNumAnimations : 0; }
    const std::map<std::string, MaraGl::BoneInfo> &GetBoneInfoMap() const { return m_BoneInfoMap; }
    int GetBoneCount() const { return m_BoneCount; }
    std::vector<MaraGl::Animation> LoadAnimations();

private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> texturesLoaded;
    Assimp::Importer m_Importer;
    const aiScene *m_Scene = nullptr;
    std::map<std::string, MaraGl::BoneInfo> m_BoneInfoMap;
    int m_BoneCount = 0;

    void loadModel(const std::string &path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);

    // Bone processing
    void ExtractBoneWeights(std::vector<Vertex> &vertices, aiMesh *mesh,
                            std::map<std::string, MaraGl::BoneInfo> &boneInfoMap,
                            int &boneCount);
    void SetVertexBoneData(Vertex &vertex, int boneID, float weight);

    std::vector<Texture> loadMaterialTextures(
        aiMaterial *mat,
        aiTextureType type,
        const std::string &typeName);

    // Debug helper to print skeleton hierarchy
    void PrintNodeHierarchy(aiNode *node, int depth) const;
};