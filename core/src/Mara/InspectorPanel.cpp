#include "InspectorPanel.h"
#include "Scene.h"
#include "HierarchyPanel.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "LightComponent.h"
#include "Model.h"
#include <memory>
#include <filesystem>

namespace MaraGl
{
    void InspectorPanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        if (!m_Scene || !m_HierarchyPanel)
        {
            ImGui::Text("Scene or HierarchyPanel not set");
            ImGui::End();
            return;
        }

        uint32_t selectedID = m_HierarchyPanel->GetSelectedEntityID();
        if (selectedID == 0)
        {
            ImGui::Text("No entity selected");
            ImGui::End();
            return;
        }

        Entity *entity = m_Scene->FindEntityByID(selectedID);
        if (!entity)
        {
            m_HierarchyPanel->SetSelectedEntityID(0);
            ImGui::Text("Selected entity no longer exists");
            ImGui::End();
            return;
        }

        ImGui::Text("Entity ID: %u", selectedID);
        ImGui::Separator();

        // Display and edit components
        auto components = m_Scene->GetRegistry().GetEntityComponents(selectedID);
        bool removeNameComponent = false;
        bool removeTransformComponent = false;
        bool removeMeshComponent = false;
        bool removeLightComponent = false;

        ImGui::Text("Components: %zu", components.size());
        ImGui::Separator();

        for (auto comp : components)
        {
            if (!comp)
                continue;

            // Get component type name
            const char *typeName = typeid(*comp).name();

            // Clean up the RTTI name (remove "struct MaraGl::" prefix if present)
            std::string cleanName = typeName;
            size_t pos = cleanName.find_last_of(':');
            if (pos != std::string::npos)
                cleanName = cleanName.substr(pos + 1);

            ImGui::PushID(comp);
            bool open = ImGui::CollapsingHeader(cleanName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            ImGui::SameLine();
            if (dynamic_cast<NameComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##name"))
                    removeNameComponent = true;
            }
            else if (dynamic_cast<TransformComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##transform"))
                    removeTransformComponent = true;
            }
            else if (dynamic_cast<MeshComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##mesh"))
                    removeMeshComponent = true;
            }
            else if (dynamic_cast<LightComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##light"))
                    removeLightComponent = true;
            }

            if (open)
            {
                comp->OnImGuiRender();

                // Special handling for MeshComponent - add button to load model
                if (auto *meshComp = dynamic_cast<MeshComponent *>(comp))
                {
                    ImGui::Spacing();

                    // Model path input
                    static char modelPathBuffer[512] = "resources/models/";
                    ImGui::InputText("Model Path##mesh", modelPathBuffer, sizeof(modelPathBuffer));

                    if (ImGui::Button("Load Model##mesh"))
                    {
                        std::string modelPath(modelPathBuffer);
                        if (std::filesystem::exists(modelPath))
                        {
                            try
                            {
                                meshComp->ModelPtr = std::make_shared<Model>(modelPath);
                                ImGui::OpenPopup("ModelLoadSuccess");
                            }
                            catch (const std::exception &e)
                            {
                                // Could display error in a popup
                            }
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Clear Model##mesh"))
                    {
                        meshComp->ModelPtr.reset();
                    }

                    if (ImGui::BeginPopup("ModelLoadSuccess"))
                    {
                        ImGui::Text("Model loaded successfully!");
                        ImGui::EndPopup();
                    }
                }
            }

            ImGui::PopID();
        }

        if (removeNameComponent)
            entity->RemoveComponent<NameComponent>();
        if (removeTransformComponent)
            entity->RemoveComponent<TransformComponent>();
        if (removeMeshComponent)
            entity->RemoveComponent<MeshComponent>();
        if (removeLightComponent)
            entity->RemoveComponent<LightComponent>();

        ImGui::Separator();

        // Add component dropdown
        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("AddComponentMenu");
        }

        if (ImGui::BeginPopup("AddComponentMenu"))
        {
            if (ImGui::MenuItem("Name"))
            {
                if (!entity->HasComponent<NameComponent>())
                {
                    auto &nameComp = entity->AddComponent<NameComponent>();
                    nameComp.Name = "New Entity";
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Transform"))
            {
                if (!entity->HasComponent<TransformComponent>())
                {
                    entity->AddComponent<TransformComponent>();
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Mesh"))
            {
                if (!entity->HasComponent<MeshComponent>())
                {
                    entity->AddComponent<MeshComponent>();
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Light"))
            {
                if (!entity->HasComponent<LightComponent>())
                {
                    entity->AddComponent<LightComponent>();
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }
}
