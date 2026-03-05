#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <imgui.h>

namespace MaraGl
{
    enum class LightType
    {
        Directional = 0,
        Point = 1,
        Spot = 2
    };

    struct LightComponent : public Component
    {
        LightType Type = LightType::Point;
        glm::vec3 Color = glm::vec3(1.0f, 1.0f, 1.0f);
        float Intensity = 1.0f;
        float Range = 10.0f;      // For point and spot lights
        float InnerAngle = 15.0f; // For spot lights (degrees)
        float OuterAngle = 25.0f; // For spot lights (degrees)
        bool Enabled = true;

        void OnImGuiRender() override
        {
            ImGui::Checkbox("Enabled##light", &Enabled);

            // Light type selector
            int type = static_cast<int>(Type);
            const char *types[] = {"Directional", "Point", "Spot"};
            if (ImGui::Combo("Type##light", &type, types, 3))
            {
                Type = static_cast<LightType>(type);
            }

            ImGui::ColorEdit3("Color##light", &Color.x);
            ImGui::DragFloat("Intensity##light", &Intensity, 0.1f, 0.0f, 5.0f);

            if (Type != LightType::Directional)
            {
                ImGui::DragFloat("Range##light", &Range, 0.5f, 0.1f, 100.0f);
            }

            if (Type == LightType::Spot)
            {
                ImGui::DragFloat("Inner Angle##light", &InnerAngle, 1.0f, 0.0f, 90.0f);
                ImGui::DragFloat("Outer Angle##light", &OuterAngle, 1.0f, 0.0f, 90.0f);
            }
        }
    };
}
