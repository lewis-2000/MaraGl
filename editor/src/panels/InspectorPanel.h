#pragma once
#include "EditorPanel.h"
#include <imgui.h>
#include <cstdint>
#include <cstddef>

namespace MaraGl
{
    class Scene;
    class HierarchyPanel;

    class InspectorPanel : public EditorPanel
    {
    public:
        InspectorPanel() : EditorPanel(), m_Scene(nullptr), m_HierarchyPanel(nullptr) {}

        const char *GetName() const override { return "Inspector"; }

        void SetScene(Scene *scene) { m_Scene = scene; }
        void SetHierarchyPanel(HierarchyPanel *panel) { m_HierarchyPanel = panel; }

        void OnImGuiRender() override;

    private:
        Scene *m_Scene;
        HierarchyPanel *m_HierarchyPanel;
    };
}