#pragma once
#include "EditorPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <glm/glm.hpp>

namespace MaraGl
{
    class Scene;
    class HierarchyPanel;

    // Keyframe for a single float property
    struct Keyframe
    {
        float time;
        float value;
    };

    // Track for animating a single property of an entity
    struct PropertyTrack
    {
        std::string propertyName; // e.g., "Position.X", "Rotation.Y"
        std::vector<Keyframe> keyframes;
    };

    // All animation tracks for a single entity
    struct EntityAnimationData
    {
        uint32_t entityID;
        std::vector<PropertyTrack> tracks;
    };

    struct TimelineState
    {
        float currentTime = 0.0f;
        float duration = 5.0f;
        bool playing = false;
    };

    class EditorTimelinePanel : public EditorPanel
    {
    public:
        EditorTimelinePanel()
            : EditorPanel(), m_Scene(nullptr), m_HierarchyPanel(nullptr) {}

        const char *GetName() const override { return "Timeline"; }

        void OnImGuiRender() override;
        void Update(float deltaTime);

        void SetScene(Scene *scene) { m_Scene = scene; }
        void SetHierarchyPanel(HierarchyPanel *panel) { m_HierarchyPanel = panel; }

    private:
        TimelineState m_Timeline;
        Scene *m_Scene;
        HierarchyPanel *m_HierarchyPanel;
        std::map<uint32_t, EntityAnimationData> m_EntityAnimations; // entity ID -> animation data
        uint32_t m_SelectedEntityID = 0;

        // Helper functions
        void RenderTimelineControls();
        void RenderEntityTracks();
        void AddKeyframe(uint32_t entityID, const std::string &propertyName, float value);
        void UpdateEntityTransforms();
        float InterpolateKeyframes(const std::vector<Keyframe> &keyframes, float time);
    };

} // namespace MaraGl