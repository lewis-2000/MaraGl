#pragma once
#include <string>
#include <memory>
#include "Transform.h"

class Model;
class Animator;

class Entity
{
public:
    Entity(uint32_t id, const std::string &name);

    uint32_t GetID() const { return m_ID; }
    const std::string &GetName() const { return m_Name; }

    Transform transform;

    void SetModel(std::shared_ptr<Model> model);
    std::shared_ptr<Model> GetModel() const;

    void SetAnimator(std::shared_ptr<Animator> animator);
    std::shared_ptr<Animator> GetAnimator() const;

private:
    uint32_t m_ID;
    std::string m_Name;

    std::shared_ptr<Model> m_Model;
    std::shared_ptr<Animator> m_Animator;
};