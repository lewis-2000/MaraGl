#pragma once
#include "EditorPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <string>
#include <map>
#include <set>
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

    // Bone sequencer track - represents one bone's animation channel
    struct BonePoseKeyframe
    {
        float time = 0.0f;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
    };

    struct BoneSequencerTrack
    {
        std::string boneName;
        bool visible = true;
        bool selected = false;
        int keyframeCount = 0;
        float minTime = 0.0f;
        float maxTime = 0.0f;
        std::vector<BonePoseKeyframe> keyframes;
    };

    // Bone sequencer state for an entity's animation
    struct BoneSequencerState
    {
        uint32_t entityID = 0;
        int animationIndex = -1; // Start at -1 so first comparison triggers discovery
        int sourceAnimationIndex = -1;
        std::string assetPath;
        float previewTime = 0.0f;
        float previewDuration = 5.0f;
        float previewSpeed = 1.0f;
        bool previewPlaying = false;
        bool previewLooping = true;
        int selectedTrackIndex = -1;
        std::vector<BoneSequencerTrack> tracks;
        std::set<std::string> discoveredBones;
        bool needsRefresh = true;
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
        std::map<uint32_t, BoneSequencerState> m_BoneSequencers;    // entity ID -> bone sequencer state
        uint32_t m_SelectedEntityID = 0;
        float m_TimelinePixelsPerSecond = 100.0f;

        // Helper functions
        void RenderTimelineControls();
        void RenderEntityTracks();
        void RenderSkeletalAnimations();
        void RenderBoneSequencer();
        void DiscoverBonesInAnimation(uint32_t entityID, int animationIndex);
        void RefreshBoneSequencerTracks(BoneSequencerState &sequencer, class AnimationComponent *animComp);
        void RenderSequencerTimeline(const char *label, float width, float height);
        void RenderBoneTrack(BoneSequencerTrack &track, const class Animation &animation, float timelineStartX, float timelineWidth);
        void AddKeyframe(uint32_t entityID, const std::string &propertyName, float value);
        void UpdateEntityTransforms();
        float InterpolateKeyframes(const std::vector<Keyframe> &keyframes, float time);
    };

} // namespace MaraGl