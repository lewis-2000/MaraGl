#include "Entity.h"
#include "Model.h"

Entity::Entity(uint32_t id, const std::string &name)
    : m_ID(id), m_Name(name)
{
}

void Entity::SetModel(std::shared_ptr<Model> model)
{
    m_Model = model;
}

std::shared_ptr<Model> Entity::GetModel() const
{
    return m_Model;
}

void Entity::SetAnimator(std::shared_ptr<Animator> animator)
{
    m_Animator = animator;
}

std::shared_ptr<Animator> Entity::GetAnimator() const
{
    return m_Animator;
}