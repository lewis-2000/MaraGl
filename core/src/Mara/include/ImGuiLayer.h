#pragma once

#include "Window.h"
#include "Framebuffer.h"
#include "Renderer.h"
#include "InspectorPanel.h"
#include "ScenePanel.h"
#include "HierarchyPanel.h"
#include "ModelLoaderPanel.h"
// #include "ConsolePanel.h"
#include "EditorTimelinePanel.h"
// #include "AssetsPanel.h"
#include "PanelManager.h"

namespace MaraGl
{
    class ImGuiLayer
    {
    public:
        ImGuiLayer(Window &window);
        ~ImGuiLayer();

        void begin();
        void end();

        void renderDockspace(Framebuffer *framebuffer, Renderer *renderer);

        ModelLoaderPanel *GetModelLoaderPanel() const { return m_ModelLoaderPanel; }
        ScenePanel *GetScenePanel() const { return m_ScenePanel; }

    private:
        void init();
        void shutdown();

    private:
        Window &m_Window;
        Framebuffer *m_Framebuffer = nullptr;
        PanelManager m_PanelManager;
        ScenePanel *m_ScenePanel = nullptr;
        ModelLoaderPanel *m_ModelLoaderPanel = nullptr;
        bool m_PanelsInitialized = false;
    };
}