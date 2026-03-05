#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Entity.h"
#include "ComponentRegistry.h"

class Shader; // Forward declaration in global namespace
class Plane;  // Forward declaration
class Skybox; // Forward declaration

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

    private:
        std::vector<std::unique_ptr<Entity>> m_Entities;
        ComponentRegistry m_Registry;
        uint32_t m_NextID = 1;
        std::unique_ptr<::Plane> m_GroundPlane;
        std::unique_ptr<::Skybox> m_Skybox;
        bool m_SkyboxEnabled = false;
    };
}