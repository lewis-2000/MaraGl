#pragma once
#include "EditorPanel.h"
#include "Framebuffer.h"
#include <cstdint>
#include <imgui.h>

namespace MaraGl
{
    class Scene;
    class HierarchyPanel;
    struct TransformComponent;
    class Renderer;

    class ScenePanel : public EditorPanel
    {
    public:
        ScenePanel(Framebuffer *fb)
            : m_Framebuffer(fb) {}

        const char *GetName() const override { return "Scene"; }

        void SetFramebuffer(Framebuffer *fb) { m_Framebuffer = fb; }
        void SetScene(Scene *scene) { m_Scene = scene; }
        void SetHierarchyPanel(HierarchyPanel *hierarchyPanel) { m_HierarchyPanel = hierarchyPanel; }
        void SetRenderer(Renderer *renderer) { m_Renderer = renderer; }

        bool IsFocused() const { return m_IsFocused; }

        void OnImGuiRender() override;

    private:
        enum class GizmoOperation
        {
            Translate,
            Scale
        };

        bool GetSelectedTransform(uint32_t &entityIDOut, TransformComponent *&transformOut) const;

        Framebuffer *m_Framebuffer;
        Scene *m_Scene = nullptr;
        HierarchyPanel *m_HierarchyPanel = nullptr;
        Renderer *m_Renderer = nullptr;
        mutable bool m_IsFocused = false;
        GizmoOperation m_Operation = GizmoOperation::Translate;
        int m_Space = 0;
    };
}