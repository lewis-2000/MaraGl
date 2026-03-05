#include "Scene.h"
#include "Renderer.h"
#include "Model.h"
#include "Shader.h"
#include "Plane.h"
#include "Skybox.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "LightComponent.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace MaraGl
{
    Scene::Scene()
        : m_GroundPlane(std::make_unique<::Plane>(20.0f, 20.0f))
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

    void Scene::Render(Renderer &renderer, ::Shader &shader)
    {
        // Render skybox first if enabled
        if (m_SkyboxEnabled && m_Skybox)
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
            skyboxShader->setMat4("view", renderer.GetCamera().GetView());
            skyboxShader->setMat4("projection", renderer.GetCamera().GetProjection());

            // Render skybox with depth cleared to max
            glDepthFunc(GL_LEQUAL);
            m_Skybox->Draw(*skyboxShader);
            glDepthFunc(GL_LESS);
        }

        // Update all lights in shader before rendering
        UpdateLightsInShader(shader);

        // Render ground plane
        shader.use();
        glm::mat4 planeModel = glm::mat4(1.0f);
        planeModel = glm::translate(planeModel, glm::vec3(0.0f, -2.0f, 0.0f));
        shader.setMat4("model", planeModel);
        shader.setMat4("view", renderer.GetCamera().GetView());
        shader.setMat4("projection", renderer.GetCamera().GetProjection());
        shader.setVec3("uViewPos", renderer.GetCamera().GetPosition());
        shader.setVec3("uObjectColor", glm::vec3(0.5f, 0.5f, 0.5f)); // Gray color for plane
        shader.setBool("uUseTexture", false);
        shader.setFloat("uAmbientStrength", renderer.GetSettings().ambientStrength);
        shader.setFloat("uSpecularStrength", renderer.GetSettings().specularStrength);
        shader.setFloat("uShininess", renderer.GetSettings().shininess);
        m_GroundPlane->Draw(shader);

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

            // Draw the model with the entity's transform
            renderer.DrawModel(*meshComp->ModelPtr, shader, transform);
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
    }

    bool Scene::LoadSkybox(const std::string &skyboxPath)
    {
        if (!m_Skybox)
            m_Skybox = std::make_unique<::Skybox>();

        bool success = m_Skybox->LoadEquirectangular(skyboxPath);
        if (success)
            m_SkyboxEnabled = true;

        return success;
    }
}
