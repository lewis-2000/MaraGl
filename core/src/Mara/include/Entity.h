#pragma once
#include <string>
#include <memory>
#include <cstdint>

namespace MaraGl
{
    class ComponentRegistry;

    class Entity
    {
    public:
        Entity(uint32_t id, ComponentRegistry *registry);

        uint32_t GetID() const { return m_ID; }

        ComponentRegistry *GetRegistry() const { return m_Registry; }

        // Template methods for component access
        template <typename T>
        T *GetComponent();

        template <typename T>
        bool HasComponent();

        template <typename T, typename... Args>
        T &AddComponent(Args &&...args);

        template <typename T>
        void RemoveComponent();

    private:
        uint32_t m_ID;
        ComponentRegistry *m_Registry;
    };
}

#include "ComponentRegistry.h"

namespace MaraGl
{
    template <typename T>
    T *Entity::GetComponent()
    {
        return m_Registry->GetComponent<T>(m_ID);
    }

    template <typename T>
    bool Entity::HasComponent()
    {
        return m_Registry->HasComponent<T>(m_ID);
    }

    template <typename T, typename... Args>
    T &Entity::AddComponent(Args &&...args)
    {
        return m_Registry->AddComponent<T>(m_ID, std::forward<Args>(args)...);
    }

    template <typename T>
    void Entity::RemoveComponent()
    {
        m_Registry->RemoveComponent<T>(m_ID);
    }
}