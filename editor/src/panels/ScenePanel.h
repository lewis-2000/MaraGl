#pragma once
#include "EditorPanel.h"
#include "Framebuffer.h"
#include <imgui.h>

namespace MaraGl
{
    class ScenePanel : public EditorPanel
    {
    public:
        ScenePanel(Framebuffer *fb)
            : m_Framebuffer(fb) {}

        const char *GetName() const override { return "Scene"; }

        void SetFramebuffer(Framebuffer *fb) { m_Framebuffer = fb; }

        bool IsFocused() const { return m_IsFocused; }

        void OnImGuiRender() override
        {
            ImGui::Begin(GetName(), &m_Open);

            // Scene input should be active whenever the viewport is hovered.
            m_IsFocused = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

            ImVec2 size = ImGui::GetContentRegionAvail();

            if (m_Framebuffer && size.x > 0 && size.y > 0)
            {
                if ((uint32_t)size.x != m_Framebuffer->getWidth() ||
                    (uint32_t)size.y != m_Framebuffer->getHeight())
                {
                    m_Framebuffer->resize(
                        (uint32_t)size.x,
                        (uint32_t)size.y);
                }

                ImGui::Image(
                    (ImTextureID)(uintptr_t)m_Framebuffer->getColorAttachment(),
                    size,
                    {0, 1}, {1, 0});
            }

            ImGui::End();
        }

    private:
        Framebuffer *m_Framebuffer;
        mutable bool m_IsFocused = false;
    };
}