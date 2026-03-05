#include "HierarchyPanel.h"
#include "Scene.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include <imgui.h>

namespace MaraGl
{

    void HierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        if (!m_Scene)
        {
            ImGui::Text("No scene loaded");
            ImGui::End();
            return;
        }

        // Keep selection valid if entity was deleted elsewhere.
        if (m_SelectedEntityID != 0 && !m_Scene->FindEntityByID(m_SelectedEntityID))
            m_SelectedEntityID = 0;

        // Create entity button
        if (ImGui::Button("+ Create Entity"))
        {
            Entity &newEntity = m_Scene->CreateEntity("New Entity");
            auto &nameComp = newEntity.AddComponent<NameComponent>();
            nameComp.Name = "New Entity";
            newEntity.AddComponent<TransformComponent>();
            m_SelectedEntityID = newEntity.GetID();
        }

        ImGui::SameLine();

        // Delete selected entity button
        if (m_SelectedEntityID != 0 && ImGui::Button("Delete Selected"))
        {
            m_Scene->DestroyEntity(m_SelectedEntityID);
            m_SelectedEntityID = 0;
        }

        ImGui::Separator();
        ImGui::Text("Entities: %zu", m_Scene->GetEntities().size());
        ImGui::Separator();

        // List all entities
        for (const auto &entity : m_Scene->GetEntities())
        {
            uint32_t id = entity->GetID();

            // Get entity name from NameComponent if available
            std::string displayName = "Entity #" + std::to_string(id);
            auto *nameComp = entity->GetComponent<NameComponent>();
            if (nameComp)
                displayName = nameComp->Name + " (ID: " + std::to_string(id) + ")";

            // Selectable for each entity
            bool isSelected = (m_SelectedEntityID == id);
            if (ImGui::Selectable(displayName.c_str(), isSelected))
            {
                m_SelectedEntityID = id;
            }
        }

        ImGui::End();
    }

}