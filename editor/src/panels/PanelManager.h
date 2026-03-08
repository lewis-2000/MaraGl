#pragma once
#include "EditorPanel.h"
#include <imgui.h>
#include <vector>
#include <memory>

namespace MaraGl
{
    class PanelManager
    {
    public:
        template <typename T, typename... Args>
        T *AddPanel(Args &&...args)
        {
            auto panel = std::make_unique<T>(std::forward<Args>(args)...);
            T *raw = panel.get();
            raw->OnAttach();
            m_Panels.push_back(std::move(panel));
            return raw;
        }

        void Update(float deltaTime)
        {
            for (auto &panel : m_Panels)
            {
                // Attempt to call Update if panel has it
                // Use dynamic_cast to check for EditorTimelinePanel specifically
                // This is a simple approach for now - Will update later to have a more robust system for panel types and updates
                if (panel)
                {
                    panel->Update(deltaTime);
                }
            }
        }

        void RenderPanels()
        {
            for (auto &panel : m_Panels)
            {
                if (!panel->IsOpen())
                    continue;

                double start = ImGui::GetTime();
                panel->OnImGuiRender();
                float ms = float((ImGui::GetTime() - start) * 1000.0);

                panel->SetLastRenderTime(ms);
            }
        }

        const std::vector<std::unique_ptr<EditorPanel>> &GetPanels() const
        {
            return m_Panels;
        }

    private:
        std::vector<std::unique_ptr<EditorPanel>> m_Panels;
    };
}