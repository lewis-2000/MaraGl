#pragma once
#include "Component.h"
#include <string>
#include <cstring>
#include <imgui.h>

namespace MaraGl
{
    struct NameComponent : public Component
    {
        std::string Name = "Entity";

        void OnImGuiRender() override
        {
            char buffer[256];
            std::strncpy(buffer, Name.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
            if (ImGui::InputText("Name##entity", buffer, 256))
            {
                Name = buffer;
            }
        }
    };
}
