#pragma once
#include "Component.h"
#include <string>
#include <imgui.h>

namespace MaraGl
{
    struct NameComponent : public Component
    {
        std::string Name = "Entity";

        void OnImGuiRender() override
        {
            char buffer[256];
            strcpy_s(buffer, sizeof(buffer), Name.c_str());
            if (ImGui::InputText("Name##entity", buffer, 256))
            {
                Name = buffer;
            }
        }
    };
}
