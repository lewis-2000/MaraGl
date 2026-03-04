#pragma once
#include <vector>
#include <memory>
#include "Entity.h"

class Scene
{
public:
    Entity &CreateEntity(const std::string &name = "Entity");
    void DestroyEntity(uint32_t id);

    void Update(float deltaTime);
    void Render(class Renderer &renderer);

    const std::vector<std::unique_ptr<Entity>> &GetEntities() const { return m_Entities; }

private:
    std::vector<std::unique_ptr<Entity>> m_Entities;
    uint32_t m_NextID = 0;
};