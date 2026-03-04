#include "Scene.h"
#include "Renderer.h"
#include "Model.h"
#include <algorithm>

namespace MaraGl
{

    Entity &Scene::CreateEntity(const std::string &name)
    {
        auto entity = std::make_unique<Entity>(m_NextID++, name);
        Entity *ptr = entity.get();
        m_Entities.push_back(std::move(entity));
        return *ptr;
    }

    void Scene::DestroyEntity(uint32_t id)
    {
        auto it = std::remove_if(m_Entities.begin(), m_Entities.end(),
                                 [id](const auto &e)
                                 { return e->GetID() == id; });
        m_Entities.erase(it, m_Entities.end());
    }

    void Scene::Update(float deltaTime)
    {
        for (auto &entity : m_Entities)
        {
            // Update animations if animator exists
            if (auto animator = entity->GetAnimator())
            {
                // animator->Update(deltaTime);
            }
        }
    }

    void Scene::Render(Renderer &renderer)
    {
        for (auto &entity : m_Entities)
        {
            if (auto model = entity->GetModel())
            {
                // renderer.DrawModel(*model, entity->transform.GetMatrix());
            }
        }
    }
}
