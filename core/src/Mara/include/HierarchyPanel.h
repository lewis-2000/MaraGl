#pragma once
#include "EditorPanel.h"
#include <cstdint>

namespace MaraGl
{
    class Scene;

    class HierarchyPanel : public EditorPanel
    {
    public:
        HierarchyPanel() : EditorPanel(), m_SelectedEntityID(0), m_Scene(nullptr) {}

        const char *GetName() const override { return "Hierarchy"; }

        void OnImGuiRender() override;

        void SetScene(Scene *scene) { m_Scene = scene; }

        uint32_t GetSelectedEntityID() const { return m_SelectedEntityID; }
        void SetSelectedEntityID(uint32_t id) { m_SelectedEntityID = id; }

    private:
        Scene *m_Scene;
        uint32_t m_SelectedEntityID;
    };

}