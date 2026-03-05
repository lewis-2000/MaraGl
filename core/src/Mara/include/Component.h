#pragma once
#include <typeindex>
#include <typeinfo>

namespace MaraGl
{
    class Component
    {
    public:
        virtual ~Component() = default;

        virtual void Update(float dt) {}
        virtual void OnImGuiRender() {}

        std::type_index GetTypeIndex() const { return std::type_index(typeid(*this)); }
    };
}
