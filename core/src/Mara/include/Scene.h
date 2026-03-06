#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "Entity.h"
#include "ComponentRegistry.h"

class Shader;     // Forward declaration in global namespace
class Plane;      // Forward declaration
class Skybox;     // Forward declaration
class LightGizmo; // Forward declaration

namespace MaraGl
{
    class Renderer;

    class Scene
    {
    public:
        Scene();
        ~Scene();

        Entity &CreateEntity(const std::string &name = "Entity");
        void DestroyEntity(uint32_t id);

        void Update(float deltaTime);
        void Render(Renderer &renderer, ::Shader &shader);

        Entity *FindEntityByID(uint32_t id);
        ComponentRegistry &GetRegistry() { return m_Registry; }

        const std::vector<std::unique_ptr<Entity>> &GetEntities() const { return m_Entities; }

        void UpdateLightsInShader(::Shader &shader);

        bool LoadSkybox(const std::string &skyboxPath);
        void SetSkyboxEnabled(bool enabled) { m_SkyboxEnabled = enabled; }
        bool IsSkyboxEnabled() const { return m_SkyboxEnabled; }

        // Overcast light (global ambient illumination)
        void SetOvercastEnabled(bool enabled) { m_OvercastEnabled = enabled; }
        bool IsOvercastEnabled() const { return m_OvercastEnabled; }
        void SetOvercastColor(const glm::vec3 &color) { m_OvercastColor = color; }
        glm::vec3 GetOvercastColor() const { return m_OvercastColor; }
        void SetOvercastIntensity(float intensity) { m_OvercastIntensity = intensity; }
        float GetOvercastIntensity() const { return m_OvercastIntensity; }

        // Scene serialization
        bool SaveToFile(const std::string &filepath);
        bool LoadFromFile(const std::string &filepath);
        void ClearScene();

    private:
        std::vector<std::unique_ptr<Entity>> m_Entities;
        ComponentRegistry m_Registry;
        uint32_t m_NextID = 1;
        std::unique_ptr<::Plane> m_GroundPlane;
        std::unique_ptr<::Skybox> m_Skybox;
        bool m_SkyboxEnabled = false;
        std::unique_ptr<::LightGizmo> m_LightGizmo;
        std::unique_ptr<::Shader> m_UnlitShader;

        // Overcast light settings
        bool m_OvercastEnabled = true;
        glm::vec3 m_OvercastColor = glm::vec3(0.6f, 0.7f, 0.8f); // Cool sky color
        float m_OvercastIntensity = 0.4f;
    };
}