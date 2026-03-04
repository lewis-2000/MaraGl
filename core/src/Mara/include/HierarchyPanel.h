#pragma once
#include "EditorPanel.h"

namespace MaraGl
{

    class HierarchyPanel : public EditorPanel
    {
    public:
        HierarchyPanel() : EditorPanel() {}

        const char *GetName() const override { return "Hierarchy"; }

        void OnImGuiRender() override;
    };

}