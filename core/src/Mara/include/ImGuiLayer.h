#pragma once

#include "Window.h"
#include "Framebuffer.h"
#include "Renderer.h"
#include "InspectorPanel.h"
#include "ScenePanel.h"
#include "HierarchyPanel.h"
#include "ModelLoaderPanel.h"
#include "SceneSettingsPanel.h"
// #include "ConsolePanel.h"
#include "EditorTimelinePanel.h"
// #include "AssetsPanel.h"
#include "PanelManager.h"

#include <imgui.h>
#include <map>
#include <string>

namespace MaraGl
{
    class Scene;

    class ImGuiLayer
    {
    public:
        ImGuiLayer(Window &window);
        ~ImGuiLayer();

        void begin();
        void end();

        void renderDockspace(Framebuffer *framebuffer, Renderer *renderer);
        void Update(float deltaTime) { m_PanelManager.Update(deltaTime); }

        ModelLoaderPanel *GetModelLoaderPanel() const { return m_ModelLoaderPanel; }
        ScenePanel *GetScenePanel() const { return m_ScenePanel; }
        HierarchyPanel *GetHierarchyPanel() const { return m_HierarchyPanel; }

        void SetScene(Scene *scene);

        void ApplyModernEditorStyle();

        // Icon font management
        ImFont *LoadIconFont(const std::string &fontPath, float fontSize, const char *fontName = "Icons");
        ImFont *GetIconFont(const char *fontName = "Icons") const;
        bool HasIconFont(const char *fontName = "Icons") const;

    private:
        void init();
        void shutdown();
        void LoadDefaultIconFonts();

    private:
        Window &m_Window;
        Framebuffer *m_Framebuffer = nullptr;
        PanelManager m_PanelManager;
        ScenePanel *m_ScenePanel = nullptr;
        ModelLoaderPanel *m_ModelLoaderPanel = nullptr;
        HierarchyPanel *m_HierarchyPanel = nullptr;
        EditorTimelinePanel *m_TimelinePanel = nullptr;
        InspectorPanel *m_InspectorPanel = nullptr;
        SceneSettingsPanel *m_SceneSettingsPanel = nullptr;
        bool m_PanelsInitialized = false;
        Scene *m_Scene = nullptr;

        // Icon font management
        std::map<std::string, ImFont *> m_IconFonts;
    };
}