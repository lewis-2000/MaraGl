#pragma once
#include "EditorPanel.h"
#include <imgui.h>

namespace MaraGl
{
    class InspectorPanel : public EditorPanel
    {
    public:
        const char *GetName() const override { return "Inspector"; }

        void OnImGuiRender() override
        {
            ImGui::Begin(GetName(), &m_Open);
            ImGui::Text("Inspector panel working.");
            ImGui::End();
        }
    };
}