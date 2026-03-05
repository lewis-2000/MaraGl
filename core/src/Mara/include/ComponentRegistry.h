#pragma once
#include "Component.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <typeindex>

namespace MaraGl
{
    class ComponentRegistry
    {
    public:
        // Add a component to an entity
        template <typename T, typename... Args>
        T &AddComponent(uint32_t entityID, Args &&...args)
        {
            auto &vec = GetComponentVector<T>();

            // Expand capacity if needed
            if (entityID >= vec.size())
                vec.resize(entityID + 1);

            vec[entityID] = std::make_unique<T>(std::forward<Args>(args)...);
            return *static_cast<T *>(vec[entityID].get());
        }

        // Get a component from an entity
        template <typename T>
        T *GetComponent(uint32_t entityID)
        {
            auto typeIdx = std::type_index(typeid(T));
            if (m_Components.find(typeIdx) == m_Components.end())
                return nullptr;

            auto &vec = m_Components[typeIdx];
            if (entityID >= vec.size() || !vec[entityID])
                return nullptr;

            return static_cast<T *>(vec[entityID].get());
        }

        // Check if entity has component
        template <typename T>
        bool HasComponent(uint32_t entityID)
        {
            return GetComponent<T>(entityID) != nullptr;
        }

        // Remove a component
        template <typename T>
        void RemoveComponent(uint32_t entityID)
        {
            auto typeIdx = std::type_index(typeid(T));
            if (m_Components.find(typeIdx) != m_Components.end())
            {
                auto &vec = m_Components[typeIdx];
                if (entityID < vec.size())
                    vec[entityID] = nullptr;
            }
        }

        // Get all components for an entity
        std::vector<Component *> GetEntityComponents(uint32_t entityID)
        {
            std::vector<Component *> components;
            for (auto &[typeIdx, vec] : m_Components)
            {
                if (entityID < vec.size() && vec[entityID])
                    components.push_back(vec[entityID].get());
            }
            return components;
        }

        // Clear all components for an entity
        void ClearEntityComponents(uint32_t entityID)
        {
            for (auto &[typeIdx, vec] : m_Components)
            {
                if (entityID < vec.size())
                    vec[entityID] = nullptr;
            }
        }

    private:
        std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>> m_Components;

        template <typename T>
        std::vector<std::unique_ptr<Component>> &GetComponentVector()
        {
            auto typeIdx = std::type_index(typeid(T));
            if (m_Components.find(typeIdx) == m_Components.end())
                m_Components[typeIdx].resize(1000); // Pre-allocate
            return m_Components[typeIdx];
        }
    };
}
