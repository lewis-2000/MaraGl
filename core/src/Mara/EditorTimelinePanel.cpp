#include "EditorTimelinePanel.h"
#include "Scene.h"
#include "HierarchyPanel.h"
#include "Entity.h"
#include "TransformComponent.h"
#include "NameComponent.h"
#include <algorithm>

namespace MaraGl
{
    void EditorTimelinePanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        if (!m_Scene)
        {
            ImGui::Text("Scene not set");
            ImGui::End();
            return;
        }

        // Get selected entity from HierarchyPanel
        if (m_HierarchyPanel)
        {
            m_SelectedEntityID = m_HierarchyPanel->GetSelectedEntityID();
        }

        RenderTimelineControls();
        ImGui::Separator();
        RenderEntityTracks();

        ImGui::End();
    }

    void EditorTimelinePanel::RenderTimelineControls()
    {
        ImGui::SeparatorText("Playback");

        if (ImGui::Button("⏮ Start"))
            m_Timeline.currentTime = 0.0f;

        ImGui::SameLine();
        if (ImGui::Button(m_Timeline.playing ? "⏸ Pause" : "▶ Play"))
            m_Timeline.playing = !m_Timeline.playing;

        ImGui::SameLine();
        if (ImGui::Button("⏭ End"))
            m_Timeline.currentTime = m_Timeline.duration;

        ImGui::SameLine();
        ImGui::Text("Time: %.2fs / %.2fs", m_Timeline.currentTime, m_Timeline.duration);

        // Scrubber
        ImGui::SliderFloat("##TimelineScrubber",
                           &m_Timeline.currentTime,
                           0.0f,
                           m_Timeline.duration,
                           "%.2f");

        ImGui::SliderFloat("Duration##timeline",
                           &m_Timeline.duration,
                           0.5f,
                           30.0f,
                           "%.2f");
    }

    void EditorTimelinePanel::RenderEntityTracks()
    {
        ImGui::SeparatorText("Entity Animation Tracks");

        if (m_SelectedEntityID == 0)
        {
            ImGui::Text("Select an entity to animate");
            return;
        }

        Entity *entity = m_Scene->FindEntityByID(m_SelectedEntityID);
        if (!entity)
        {
            ImGui::Text("Selected entity not found");
            return;
        }

        // Display entity name
        auto nameComp = entity->GetComponent<NameComponent>();
        ImGui::Text("Entity: %s (ID: %u)",
                    nameComp ? nameComp->Name.c_str() : "Unknown",
                    m_SelectedEntityID);

        ImGui::Separator();

        // Get or create animation data for this entity
        auto &animData = m_EntityAnimations[m_SelectedEntityID];
        if (animData.entityID == 0)
            animData.entityID = m_SelectedEntityID;

        // Transform component controls
        auto *transform = entity->GetComponent<TransformComponent>();
        if (transform)
        {
            ImGui::Text("Transform Channel:");

            ImGui::DragFloat3("Position", glm::value_ptr(transform->Position), 0.1f);

            // Add keyframe for position
            if (ImGui::Button("+ Key Position"))
            {
                AddKeyframe(m_SelectedEntityID, "Position.X", transform->Position.x);
                AddKeyframe(m_SelectedEntityID, "Position.Y", transform->Position.y);
                AddKeyframe(m_SelectedEntityID, "Position.Z", transform->Position.z);
            }

            ImGui::DragFloat3("Rotation", glm::value_ptr(transform->Rotation), 1.0f);

            if (ImGui::Button("+ Key Rotation"))
            {
                AddKeyframe(m_SelectedEntityID, "Rotation.X", transform->Rotation.x);
                AddKeyframe(m_SelectedEntityID, "Rotation.Y", transform->Rotation.y);
                AddKeyframe(m_SelectedEntityID, "Rotation.Z", transform->Rotation.z);
            }

            ImGui::DragFloat3("Scale", glm::value_ptr(transform->Scale), 0.1f);

            if (ImGui::Button("+ Key Scale"))
            {
                AddKeyframe(m_SelectedEntityID, "Scale.X", transform->Scale.x);
                AddKeyframe(m_SelectedEntityID, "Scale.Y", transform->Scale.y);
                AddKeyframe(m_SelectedEntityID, "Scale.Z", transform->Scale.z);
            }
        }

        ImGui::Separator();
        ImGui::Text("Keyframe Tracks (%zu)", animData.tracks.size());

        // Display all tracks for this entity
        for (size_t i = 0; i < animData.tracks.size(); ++i)
        {
            auto &track = animData.tracks[i];

            if (ImGui::TreeNode((void *)(intptr_t)i, "%s (%zu keys)", track.propertyName.c_str(), track.keyframes.size()))
            {
                // Display keyframes
                for (size_t k = 0; k < track.keyframes.size(); ++k)
                {
                    ImGui::Text("  Key %zu: t=%.2fs, value=%.4f",
                                k,
                                track.keyframes[k].time,
                                track.keyframes[k].value);
                }

                if (ImGui::Button(("Clear Track##" + std::to_string(i)).c_str()))
                {
                    track.keyframes.clear();
                }

                ImGui::TreePop();
            }
        }
    }

    void EditorTimelinePanel::Update(float deltaTime)
    {
        if (!m_Scene)
            return;

        // Update timeline playback
        if (m_Timeline.playing)
        {
            m_Timeline.currentTime += deltaTime;
            if (m_Timeline.currentTime > m_Timeline.duration)
            {
                m_Timeline.currentTime = 0.0f; // Loop
            }
        }

        // Update entity transforms from timeline
        UpdateEntityTransforms();
    }

    void EditorTimelinePanel::UpdateEntityTransforms()
    {
        for (auto &[entityID, animData] : m_EntityAnimations)
        {
            Entity *entity = m_Scene->FindEntityByID(entityID);
            if (!entity)
                continue;

            auto *transform = entity->GetComponent<TransformComponent>();
            if (!transform)
                continue;

            // Update position
            for (auto &track : animData.tracks)
            {
                float value = InterpolateKeyframes(track.keyframes, m_Timeline.currentTime);

                if (track.propertyName == "Position.X")
                    transform->Position.x = value;
                else if (track.propertyName == "Position.Y")
                    transform->Position.y = value;
                else if (track.propertyName == "Position.Z")
                    transform->Position.z = value;
                else if (track.propertyName == "Rotation.X")
                    transform->Rotation.x = value;
                else if (track.propertyName == "Rotation.Y")
                    transform->Rotation.y = value;
                else if (track.propertyName == "Rotation.Z")
                    transform->Rotation.z = value;
                else if (track.propertyName == "Scale.X")
                    transform->Scale.x = value;
                else if (track.propertyName == "Scale.Y")
                    transform->Scale.y = value;
                else if (track.propertyName == "Scale.Z")
                    transform->Scale.z = value;
            }
        }
    }

    void EditorTimelinePanel::AddKeyframe(uint32_t entityID, const std::string &propertyName, float value)
    {
        auto &animData = m_EntityAnimations[entityID];
        if (animData.entityID == 0)
            animData.entityID = entityID;

        // Find or create track
        PropertyTrack *track = nullptr;
        for (auto &t : animData.tracks)
        {
            if (t.propertyName == propertyName)
            {
                track = &t;
                break;
            }
        }

        if (!track)
        {
            animData.tracks.push_back(PropertyTrack{propertyName, {}});
            track = &animData.tracks.back();
        }

        // Add keyframe, replacing if at same time
        bool found = false;
        for (auto &key : track->keyframes)
        {
            if (key.time == m_Timeline.currentTime)
            {
                key.value = value;
                found = true;
                break;
            }
        }

        if (!found)
        {
            track->keyframes.push_back({m_Timeline.currentTime, value});
            // Keep keyframes sorted by time
            std::sort(track->keyframes.begin(), track->keyframes.end(),
                      [](const Keyframe &a, const Keyframe &b)
                      { return a.time < b.time; });
        }
    }

    float EditorTimelinePanel::InterpolateKeyframes(const std::vector<Keyframe> &keyframes, float time)
    {
        if (keyframes.empty())
            return 0.0f;

        // Find the two keyframes to interpolate between
        if (time <= keyframes.front().time)
            return keyframes.front().value;

        if (time >= keyframes.back().time)
            return keyframes.back().value;

        for (size_t i = 0; i < keyframes.size() - 1; ++i)
        {
            const auto &key1 = keyframes[i];
            const auto &key2 = keyframes[i + 1];

            if (time >= key1.time && time <= key2.time)
            {
                // Linear interpolation
                float t = (time - key1.time) / (key2.time - key1.time);
                return key1.value + (key2.value - key1.value) * t;
            }
        }

        return keyframes.back().value;
    }

} // namespace MaraGl