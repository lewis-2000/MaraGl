#pragma once
#include "EditorPanel.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <string>

namespace MaraGl
{

    struct TimelineKeyframe
    {
        float time; // seconds
    };

    struct TimelineTrack
    {
        std::string name;
        std::vector<TimelineKeyframe> keys;
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
            : EditorPanel() {}

        const char *GetName() const override { return "Timeline"; }

        void OnImGuiRender() override;

    private:
        TimelineState m_Timeline;
        std::vector<TimelineTrack> m_Tracks = {
            {"Position", {{0.0f}, {1.0f}, {3.5f}}},
            {"Rotation", {{0.5f}, {2.0f}}},
            {"Scale", {{0.0f}, {4.0f}}}};
    };

} // namespace MaraGl