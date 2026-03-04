#include "HierarchyPanel.h"
#include <imgui.h>

namespace MaraGl
{

    void HierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);
        ImGui::Text("Scene hierarchy goes here...");
        // Placeholder: later list ECS entities
        ImGui::End();
    }

}