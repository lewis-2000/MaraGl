#pragma once
#include <string>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui_internal.h>

namespace MaraGl
{
    struct PanelStats
    {
        float lastRenderMs = 0.0f;
        size_t memoryBytes = 0;
    };

    class EditorPanel
    {
    public:
        virtual ~EditorPanel() = default;

        virtual const char *GetName() const = 0;
        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnImGuiRender() = 0;

        bool IsOpen() const { return m_Open; }
        void SetOpen(bool open) { m_Open = open; }

        const PanelStats &GetStats() const { return m_Stats; }

        void SetLastRenderTime(float ms)
        {
            m_Stats.lastRenderMs = ms;
        }

    protected:
        bool m_Open = true;
        std::string m_Name;

    private:
        PanelStats m_Stats;
    };
}