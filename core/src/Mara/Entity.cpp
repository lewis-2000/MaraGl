#include "Entity.h"
#include "ComponentRegistry.h"

namespace MaraGl
{
    Entity::Entity(uint32_t id, ComponentRegistry *registry)
        : m_ID(id), m_Registry(registry)
    {
    }
}
