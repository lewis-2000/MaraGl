#include "EditorTimelinePanel.h"

namespace MaraGl
{

    void EditorTimelinePanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        // =============================
        // Playback controls
        // =============================
        ImGui::SeparatorText("Timeline");

        if (ImGui::Button("⏮"))
            m_Timeline.currentTime = 0.0f;

        ImGui::SameLine();
        if (ImGui::Button(m_Timeline.playing ? "⏸" : "▶"))
            m_Timeline.playing = !m_Timeline.playing;

        ImGui::SameLine();
        if (ImGui::Button("⏭"))
            m_Timeline.currentTime = m_Timeline.duration;

        ImGui::SameLine();
        ImGui::Text("Time: %.2fs", m_Timeline.currentTime);

        // Scrubber and duration
        ImGui::SliderFloat("##TimelineScrubber",
                           &m_Timeline.currentTime,
                           0.0f,
                           m_Timeline.duration,
                           "%.2fs");

        ImGui::SliderFloat("Duration",
                           &m_Timeline.duration,
                           0.5f,
                           30.0f);

        ImGui::Separator();

        // Track layout
        ImGui::Columns(2, "TimelineColumns", true);
        ImGui::SetColumnWidth(0, 140.0f);

        // Left column: track names
        for (auto &track : m_Tracks)
        {
            ImGui::TextUnformatted(track.name.c_str());
        }

        ImGui::NextColumn();

        // Right column: keyframe lanes
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorStart = ImGui::GetCursorScreenPos();
        float laneWidth = ImGui::GetContentRegionAvail().x;
        float laneHeight = 22.0f;

        float timelineTop = cursorStart.y;
        float timelineBottom = timelineTop + laneHeight * m_Tracks.size();

        ImVec2 cursor = cursorStart;

        for (auto &track : m_Tracks)
        {
            // Track line
            drawList->AddLine(
                {cursor.x, cursor.y + laneHeight * 0.5f},
                {cursor.x + laneWidth, cursor.y + laneHeight * 0.5f},
                IM_COL32(130, 130, 130, 255),
                1.0f);

            // Keyframes
            for (auto &key : track.keys)
            {
                float t = key.time / m_Timeline.duration;
                float x = cursor.x + t * laneWidth;

                drawList->AddCircleFilled(
                    {x, cursor.y + laneHeight * 0.5f},
                    4.0f,
                    IM_COL32(255, 200, 0, 255));
            }

            ImGui::Dummy(ImVec2(laneWidth, laneHeight));
            cursor.y += laneHeight;
        }

        // Playhead
        float playheadT = m_Timeline.currentTime / m_Timeline.duration;
        float playheadX = cursorStart.x + playheadT * laneWidth;

        drawList->AddLine(
            {playheadX, timelineTop},
            {playheadX, timelineBottom},
            IM_COL32(255, 60, 60, 255),
            2.0f);

        ImGui::Columns(1);
        ImGui::End();
    }

} // namespace MaraGl