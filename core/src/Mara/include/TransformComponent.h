#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

namespace MaraGl
{
    struct TransformComponent : public Component
    {
        glm::vec3 Position{0.0f};
        glm::vec3 Rotation{0.0f}; // In degrees
        glm::vec3 Scale{1.0f};

        void OnImGuiRender() override
        {
            ImGui::DragFloat3("Position##transform", glm::value_ptr(Position), 0.1f);
            ImGui::DragFloat3("Rotation##transform", glm::value_ptr(Rotation), 1.0f);
            ImGui::DragFloat3("Scale##transform", glm::value_ptr(Scale), 0.1f);
        }

        glm::mat4 GetMatrix() const
        {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), Position);

            // Apply rotations in XYZ order (degrees to radians)
            model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0, 0, 1));

            model = glm::scale(model, Scale);
            return model;
        }
    };
}
