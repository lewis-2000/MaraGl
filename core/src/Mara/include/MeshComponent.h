#pragma once
#include "Component.h"
#include "Mesh.h"
#include <memory>
#include <imgui.h>

class Model; // Forward declaration - Model is in global namespace

namespace MaraGl
{
    struct MeshComponent : public Component
    {
        std::shared_ptr<::Model> ModelPtr;
        std::string ModelPath; // Path to the model file for serialization
        bool Visible = true;
        float ModelScale = 1.0f; // Per-model scale multiplier
        std::vector<MeshMaterialOverride> MeshMaterials;

        void EnsureMeshMaterialCount(size_t meshCount)
        {
            MeshMaterials.resize(meshCount);
        }

        void OnImGuiRender() override
        {
            ImGui::Checkbox("Visible##mesh", &Visible);
            ImGui::DragFloat("Scale##model", &ModelScale, 0.1f, 0.01f, 100.0f);

            if (ModelPtr)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Model Loaded");
                ImGui::Text("Model is ready to render");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "No Model");
                ImGui::Text("Use the input below to load a model");
            }
        }
    };
}
