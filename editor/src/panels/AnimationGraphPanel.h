#pragma once

#include "EditorPanel.h"
#include "Scene.h"
#include "HierarchyPanel.h"
#include "AnimationComponent.h"
#include "NameComponent.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <future>

namespace MaraGl
{
    class AnimationGraphPanel : public EditorPanel
    {
    public:
        const char *GetName() const override { return "Animation Graph"; }

        void SetScene(Scene *scene) { m_Scene = scene; }
        void SetHierarchyPanel(HierarchyPanel *hierarchy) { m_Hierarchy = hierarchy; }
        void OnImGuiRender() override;

    private:
        void RefreshAnimationFiles();
        void DrawGraphCanvas(AnimationComponent *anim);
        void DrawLibraryPane(AnimationComponent *anim);
        void DrawInspectorPane(AnimationComponent *anim);
        void DrawStatusBar(AnimationComponent *anim);
        void ExtractGraphFromFiles(AnimationComponent *anim, const std::vector<std::string> &files, bool replaceExisting);
        void StartLibraryImport(const std::vector<std::string> &files, uint32_t entityId);
        void PumpLibraryImport(AnimationComponent *anim, uint32_t entityId);
        static const char *KeyToLabel(int key);

        Scene *m_Scene = nullptr;
        HierarchyPanel *m_Hierarchy = nullptr;
        std::vector<std::string> m_AnimationFiles;
        bool m_FilesScanned = false;
        int m_SelectedImportFile = 0;
        int m_SelectedStateIndex = -1;
        int m_SelectedTransitionIndex = -1;
        bool m_RequestAutoLayout = true;
        bool m_RequestFitView = false;
        bool m_RequestRestorePositions = true;
        int m_RequestFocusNodeId = -1;
        uint32_t m_LastEntityId = 0;
        size_t m_LastLaidOutStateCount = 0;
        uint32_t m_ImportTargetEntityId = 0;
        bool m_ImportInProgress = false;
        int m_ImportAddedCount = 0;
        std::future<std::vector<AnimationComponent::AnimationLibraryEntry>> m_ImportFuture;
        char m_StateSearch[128] = {};
        char m_LibraryFilter[128] = {};
    };
}
