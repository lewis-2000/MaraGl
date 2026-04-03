#include "Scene.h"
#include "Renderer.h"
#include "Model.h"
#include "Shader.h"
#include "Plane.h"
#include "Skybox.h"
#include "LightGizmo.h"
#include "SceneSerializer.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "LightComponent.h"
#include "AnimationComponent.h"
#include "Animator.h"
#include <algorithm>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace MaraGl
{
    Scene::Scene()
        : m_GroundPlane(std::make_unique<::Plane>(100.0f, 100.0f)),
          m_LightGizmo(std::make_unique<::LightGizmo>(0.3f, 16)),
          m_UnlitShader(std::make_unique<::Shader>("resources/shaders/unlit.vert", "resources/shaders/unlit.frag")),
          m_GridShader(std::make_unique<::Shader>("resources/shaders/grid.vert", "resources/shaders/grid.frag"))
    {
    }

    Scene::~Scene() = default;

    Entity &Scene::CreateEntity(const std::string &name)
    {
        auto entity = std::make_unique<Entity>(m_NextID++, &m_Registry);
        Entity *ptr = entity.get();
        m_Entities.push_back(std::move(entity));
        return *ptr;
    }

    void Scene::DestroyEntity(uint32_t id)
    {
        auto it = std::remove_if(m_Entities.begin(), m_Entities.end(),
                                 [id](const auto &e)
                                 { return e->GetID() == id; });
        if (it != m_Entities.end())
        {
            m_Entities.erase(it, m_Entities.end());
            m_Registry.ClearEntityComponents(id);
        }
    }

    Entity *Scene::FindEntityByID(uint32_t id)
    {
        for (auto &entity : m_Entities)
        {
            if (entity->GetID() == id)
                return entity.get();
        }
        return nullptr;
    }

    void Scene::Update(float deltaTime)
    {
        // Update animations
        for (auto &entity : m_Entities)
        {
            auto *animComp = entity->GetComponent<AnimationComponent>();
            if (animComp && !animComp->animations.empty())
            {
                int currentAnimIndex = animComp->currentAnimation;
                if (currentAnimIndex >= 0 && currentAnimIndex < static_cast<int>(animComp->animations.size()))
                {
                    MaraGl::Animation &animation = animComp->animations[currentAnimIndex];

                    auto *meshComp = entity->GetComponent<MeshComponent>();
                    if (meshComp && meshComp->ModelPtr)
                    {
                        const aiScene *scene = meshComp->ModelPtr->GetScene();
                        if (scene && scene->mRootNode)
                        {
                            if (animComp->playing)
                                Animator::UpdateAnimation(animComp, deltaTime);

                            // Initialize all bone transforms to identity before calculating
                            // Bones not found in hierarchy will remain as identity
                            for (auto &transform : animComp->boneTransforms)
                            {
                                transform = glm::mat4(1.0f);
                            }

                            glm::mat4 identity(1.0f);
                            glm::mat4 globalInverseTransform = Animator::GetGlobalInverseTransform(scene);

                            if (animComp->isBlending && animComp->IsValidAnimationIndex(animComp->nextAnimation))
                            {
                                const Animation &nextAnimation = animComp->animations[static_cast<size_t>(animComp->nextAnimation)];
                                const float blendFactor = (animComp->blendDuration > 0.0001f)
                                                              ? std::clamp(animComp->blendTime / animComp->blendDuration, 0.0f, 1.0f)
                                                              : 1.0f;
                                Animator::CalculateBoneTransform(animComp,
                                                                 animation,
                                                                 &nextAnimation,
                                                                 scene->mRootNode,
                                                                 identity,
                                                                 globalInverseTransform,
                                                                 blendFactor);
                            }
                            else
                            {
                                Animator::CalculateBoneTransform(animComp, animation, scene->mRootNode, identity, globalInverseTransform);
                            }
                        }
                    }
                }
            }
        }

        // Update all entities
        for (auto &entity : m_Entities)
        {
            // Update components for this entity
            auto components = m_Registry.GetEntityComponents(entity->GetID());
            for (auto comp : components)
            {
                if (comp)
                    comp->Update(deltaTime);
            }
        }
    }

    void Scene::RenderGrid(Renderer &renderer)
    {
        if (!m_GridShader || !m_GroundPlane)
            return;

        // Keep grid centered around the camera in XZ so it feels infinite.
        m_GridShader->use();
        glm::vec3 camPos = renderer.GetCamera().GetPosition();
        glm::mat4 planeModel = glm::mat4(1.0f);
        planeModel = glm::translate(planeModel, glm::vec3(camPos.x, 0.0f, camPos.z));
        m_GridShader->setMat4("model", planeModel);
        m_GridShader->setMat4("view", renderer.GetCamera().GetView());
        m_GridShader->setMat4("projection", renderer.GetCamera().GetProjection());
        m_GridShader->setVec3("uViewPos", camPos);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_GroundPlane->Draw(*m_GridShader);
        glDisable(GL_BLEND);
    }

    void Scene::Render(Renderer &renderer, ::Shader &shader)
    {
        static int s_SkyboxSkipLogCounter = 0;

        // Render skybox first if enabled
        if (m_SkyboxEnabled && m_Skybox)
        {
            const bool hasAnySkyboxTexture = (m_Skybox->GetEquirectTexture() != 0) || (m_Skybox->GetCubemapTexture() != 0);
            if (!hasAnySkyboxTexture)
            {
                static bool s_LoggedNoSkyboxTexture = false;
                if (!s_LoggedNoSkyboxTexture)
                {
                    std::cerr << "[Skybox] Enabled but no GPU texture is loaded. Skipping draw." << std::endl;
                    s_LoggedNoSkyboxTexture = true;
                }
            }
            else
            {
                // Create appropriate skybox shader
                static ::Shader *cubemapShader = nullptr;
                static ::Shader *equirectShader = nullptr;

                ::Shader *skyboxShader = nullptr;
                if (m_Skybox->IsEquirectangular())
                {
                    if (!equirectShader)
                        equirectShader = new ::Shader("resources/shaders/skybox.vert", "resources/shaders/skybox_equirect.frag");
                    skyboxShader = equirectShader;
                }
                else
                {
                    if (!cubemapShader)
                        cubemapShader = new ::Shader("resources/shaders/skybox.vert", "resources/shaders/skybox.frag");
                    skyboxShader = cubemapShader;
                }

                skyboxShader->use();
                // Remove translation so the skybox remains infinitely distant.
                glm::mat4 skyboxView = glm::mat4(glm::mat3(renderer.GetCamera().GetView()));
                skyboxShader->setMat4("view", skyboxView);
                skyboxShader->setMat4("projection", renderer.GetCamera().GetProjection());

                // Set exposure for HDR tone mapping (adjust this value to control brightness)
                if (m_Skybox->IsEquirectangular())
                {
                    skyboxShader->setFloat("exposure", 1.0f);
                }

                static bool s_LoggedSkyboxRenderInfo = false;
                if (!s_LoggedSkyboxRenderInfo)
                {
                    std::cout << "[Skybox] Render path active. type="
                              << (m_Skybox->IsEquirectangular() ? "equirect" : "cubemap")
                              << " equirectTex=" << m_Skybox->GetEquirectTexture()
                              << " cubemapTex=" << m_Skybox->GetCubemapTexture()
                              << " enabled=" << m_SkyboxEnabled
                              << std::endl;
                    s_LoggedSkyboxRenderInfo = true;
                }

                // Render skybox with depth cleared to max
                glDepthFunc(GL_LEQUAL);
                m_Skybox->Draw(*skyboxShader);
                glDepthFunc(GL_LESS);
            }
        }
        else if ((s_SkyboxSkipLogCounter++ % 180) == 0)
        {
            std::cout << "[Skybox] Skipped render. enabled=" << m_SkyboxEnabled
                      << " hasSkyboxObj=" << (m_Skybox ? 1 : 0)
                      << std::endl;
        }

        // Update all lights in shader before rendering
        UpdateLightsInShader(shader);

        // Render all entities with MeshComponent
        for (auto &entity : m_Entities)
        {
            auto *meshComp = entity->GetComponent<MeshComponent>();
            if (!meshComp || !meshComp->Visible || !meshComp->ModelPtr)
                continue;

            auto *transformComp = entity->GetComponent<TransformComponent>();
            if (!transformComp)
                continue;

            // Get the transform matrix from the component
            glm::mat4 transform = transformComp->GetMatrix();

            // Apply per-model scale multiplier
            transform = glm::scale(transform, glm::vec3(meshComp->ModelScale));

            // Check if entity has animation component and set bone transforms
            auto *animComp = entity->GetComponent<AnimationComponent>();
            if (animComp && animComp->playing && !animComp->boneTransforms.empty())
            {
                constexpr size_t kShaderMaxBones = 100;
                shader.use();
                shader.setBool("uUseAnimation", true);

                if (animComp->boneTransforms.size() > kShaderMaxBones)
                {
                    std::vector<glm::mat4> clipped(animComp->boneTransforms.begin(),
                                                   animComp->boneTransforms.begin() + kShaderMaxBones);
                    shader.setMat4Array("uBoneTransforms", clipped);
                }
                else
                {
                    shader.setMat4Array("uBoneTransforms", animComp->boneTransforms);
                }
            }
            else
            {
                shader.use();
                shader.setBool("uUseAnimation", false);
            }

            // Draw every visible mesh each frame.
            if (meshComp->MeshMaterials.size() != meshComp->ModelPtr->GetMeshCount())
            {
                meshComp->EnsureMeshMaterialCount(meshComp->ModelPtr->GetMeshCount());
            }

            renderer.DrawModel(*meshComp->ModelPtr, shader, transform, &meshComp->MeshMaterials);
        }

        // Render light gizmos (editor visualization)
        if (m_LightGizmoVisible && m_LightGizmo && m_UnlitShader)
        {
            m_UnlitShader->use();
            m_UnlitShader->setMat4("view", renderer.GetCamera().GetView());
            m_UnlitShader->setMat4("projection", renderer.GetCamera().GetProjection());

            for (auto &entity : m_Entities)
            {
                auto *lightComp = entity->GetComponent<LightComponent>();
                if (!lightComp)
                    continue;

                auto *transformComp = entity->GetComponent<TransformComponent>();
                if (!transformComp)
                    continue;

                glm::mat4 gizmoModel = glm::mat4(1.0f);
                gizmoModel = glm::translate(gizmoModel, transformComp->Position);

                m_UnlitShader->setMat4("model", gizmoModel);
                m_UnlitShader->setVec3("uColor", lightComp->Color);
                m_UnlitShader->setFloat("uIntensity", lightComp->Intensity * 2.0f); // Boost for visibility

                m_LightGizmo->Draw(*m_UnlitShader);
            }
        }
    }

    void Scene::UpdateLightsInShader(::Shader &shader)
    {
        shader.use();

        int pointLightCount = 0;
        int spotLightCount = 0;
        glm::vec3 dirLightDir = glm::vec3(0.0f);
        glm::vec3 dirLightColor = glm::vec3(0.0f);
        float dirLightIntensity = 0.0f;

        // Collect all lights from entities
        for (auto &entity : m_Entities)
        {
            auto *lightComp = entity->GetComponent<LightComponent>();
            if (!lightComp || !lightComp->Enabled)
                continue;

            auto *transformComp = entity->GetComponent<TransformComponent>();
            glm::vec3 lightPos = transformComp ? transformComp->Position : glm::vec3(0.0f);
            glm::vec3 lightDir = transformComp ? glm::normalize(-transformComp->Position) : glm::vec3(0.0f, -1.0f, 0.0f);

            if (lightComp->Type == LightType::Directional)
            {
                dirLightDir = lightDir;
                dirLightColor = lightComp->Color;
                dirLightIntensity = lightComp->Intensity;
            }
            else if (lightComp->Type == LightType::Point && pointLightCount < 4)
            {
                std::string baseName = "uPointLights[" + std::to_string(pointLightCount) + "]";
                shader.setVec3((baseName + ".position").c_str(), lightPos);
                shader.setVec3((baseName + ".color").c_str(), lightComp->Color);
                shader.setFloat((baseName + ".intensity").c_str(), lightComp->Intensity);
                shader.setFloat((baseName + ".range").c_str(), lightComp->Range);
                pointLightCount++;
            }
            else if (lightComp->Type == LightType::Spot && spotLightCount < 2)
            {
                std::string baseName = "uSpotLights[" + std::to_string(spotLightCount) + "]";
                shader.setVec3((baseName + ".position").c_str(), lightPos);
                shader.setVec3((baseName + ".direction").c_str(), lightDir);
                shader.setVec3((baseName + ".color").c_str(), lightComp->Color);
                shader.setFloat((baseName + ".intensity").c_str(), lightComp->Intensity);
                shader.setFloat((baseName + ".range").c_str(), lightComp->Range);
                shader.setFloat((baseName + ".innerCutoff").c_str(), glm::cos(glm::radians(lightComp->InnerAngle)));
                shader.setFloat((baseName + ".outerCutoff").c_str(), glm::cos(glm::radians(lightComp->OuterAngle)));
                spotLightCount++;
            }
        }

        // Pass counts and directional light
        shader.setInt("uPointLightCount", pointLightCount);
        shader.setInt("uSpotLightCount", spotLightCount);
        shader.setVec3("uDirLight.direction", dirLightDir);
        shader.setVec3("uDirLight.color", dirLightColor);
        shader.setFloat("uDirLight.intensity", dirLightIntensity);

        // Overcast light (global ambient)
        shader.setBool("uOvercastEnabled", m_OvercastEnabled);
        shader.setVec3("uOvercastColor", m_OvercastColor);
        shader.setFloat("uOvercastIntensity", m_OvercastIntensity);
    }

    bool Scene::LoadSkybox(const std::string &skyboxPath)
    {
        if (!m_Skybox)
            m_Skybox = std::make_unique<::Skybox>();

        std::cout << "[Skybox] Loading equirect path: " << skyboxPath << std::endl;

        bool success = m_Skybox->LoadEquirectangular(skyboxPath);
        if (success)
        {
            m_SkyboxEnabled = true;
            m_SkyboxPath = skyboxPath;
            std::cout << "[Skybox] Load success. equirectTex=" << m_Skybox->GetEquirectTexture()
                      << " enabled=" << m_SkyboxEnabled << std::endl;
        }
        else
        {
            m_SkyboxEnabled = false;
            m_SkyboxPath.clear();
            std::cerr << "[Skybox] Load failed for path: " << skyboxPath << std::endl;
        }

        return success;
    }

    bool Scene::SaveToFile(const std::string &filepath)
    {
        return SceneSerializer::SaveScene(this, filepath);
    }

    bool Scene::LoadFromFile(const std::string &filepath)
    {
        ClearScene();
        return SceneSerializer::LoadScene(this, filepath);
    }

    void Scene::ClearScene()
    {
        // Clear all entities (this will also clear components in the registry)
        for (auto &entity : m_Entities)
        {
            m_Registry.ClearEntityComponents(entity->GetID());
        }
        m_Entities.clear();
        m_NextID = 1;
        m_Skybox.reset();
        m_SkyboxEnabled = false;
        m_SkyboxPath.clear();
    }
}
