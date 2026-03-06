#include "EditorTimelinePanel.h"
#include "Scene.h"
#include "HierarchyPanel.h"
#include "Entity.h"
#include "TransformComponent.h"
#include "NameComponent.h"
#include "AnimationComponent.h"
#include "MeshComponent.h"
#include "Model.h"
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
        RenderSkeletalAnimations();
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
        if (ImGui::Button(m_Timeline.playing ? "⏸ Pause##timeline" : "▶ Play##timeline"))
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

    void EditorTimelinePanel::RenderSkeletalAnimations()
    {
        ImGui::SeparatorText("Skeletal Animations");

        if (m_SelectedEntityID == 0)
        {
            ImGui::Text("Select an entity to view animations");
            return;
        }

        Entity *entity = m_Scene->FindEntityByID(m_SelectedEntityID);
        if (!entity)
        {
            ImGui::Text("Selected entity not found");
            return;
        }

        // Check if entity has AnimationComponent
        auto *animComp = entity->GetComponent<AnimationComponent>();
        if (!animComp)
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No AnimationComponent on this entity");

            // Show hint if entity has a mesh that might have animations
            auto *meshComp = entity->GetComponent<MeshComponent>();
            if (meshComp && meshComp->ModelPtr && meshComp->ModelPtr->HasAnimations())
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                                   "✓ Model has %d animation(s)", meshComp->ModelPtr->GetAnimationCount());
                ImGui::TextWrapped("Add AnimationComponent in Inspector and click 'Load Animations from Model'");
            }
            return;
        }

        // Animation component exists
        if (animComp->animations.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No animations loaded");
            ImGui::TextWrapped("Load animations from Inspector panel");
            return;
        }

        // Animation info
        ImGui::Text("Loaded Animations: %zu", animComp->animations.size());
        ImGui::Text("Active Bones: %zu", animComp->boneInfoMap.size());

        ImGui::Separator();

        // Animation selector
        Animation &currentAnim = animComp->animations[animComp->currentAnimation];
        if (ImGui::BeginCombo("Select Animation", currentAnim.name.c_str()))
        {
            for (size_t i = 0; i < animComp->animations.size(); i++)
            {
                bool isSelected = (animComp->currentAnimation == static_cast<int>(i));
                if (ImGui::Selectable(animComp->animations[i].name.c_str(), isSelected))
                {
                    animComp->currentAnimation = static_cast<int>(i);
                    animComp->currentTime = 0.0f;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Playback controls
        ImGui::Spacing();

        if (ImGui::Button(animComp->playing ? "⏸ Pause##skeletal" : "▶ Play##skeletal", ImVec2(80, 0)))
            animComp->playing = !animComp->playing;

        ImGui::SameLine();
        if (ImGui::Button("⏹ Stop", ImVec2(80, 0)))
        {
            animComp->playing = false;
            animComp->currentTime = 0.0f;
        }

        ImGui::SameLine();
        ImGui::Checkbox("Loop", &animComp->looping);

        // Time display
        float duration = currentAnim.duration / currentAnim.ticksPerSecond;
        float displayTime = (animComp->currentTime / currentAnim.ticksPerSecond);
        ImGui::Text("Time: %.2fs / %.2fs", displayTime, duration);

        // Time scrubber (in seconds for user-friendliness)
        float scrubTime = displayTime;
        if (ImGui::SliderFloat("##bone_anim_scrub", &scrubTime, 0.0f, duration, "%.2fs"))
        {
            animComp->currentTime = scrubTime * currentAnim.ticksPerSecond;
            animComp->playing = false; // Pause when manually scrubbing
        }

        // Playback speed
        ImGui::SliderFloat("Speed##skeletal", &animComp->playbackSpeed, 0.1f, 3.0f, "%.2fx");

        // Sync with timeline option
        ImGui::Spacing();
        static bool syncWithTimeline = false;
        if (ImGui::Checkbox("Sync with Timeline", &syncWithTimeline))
        {
            if (syncWithTimeline)
            {
                // Sync timeline to animation
                m_Timeline.currentTime = displayTime;
                m_Timeline.duration = duration;
                m_Timeline.playing = animComp->playing;
            }
        }

        if (syncWithTimeline)
        {
            // Keep them in sync
            animComp->playing = m_Timeline.playing;
            if (m_Timeline.playing)
            {
                float syncedTime = m_Timeline.currentTime * currentAnim.ticksPerSecond;
                animComp->currentTime = syncedTime;
            }
            else
            {
                m_Timeline.currentTime = displayTime;
            }
        }

        // Animation details (collapsible)
        ImGui::Spacing();
        if (ImGui::TreeNode("Animation Details"))
        {
            ImGui::Text("Duration: %.2fs (%.0f ticks)", duration, currentAnim.duration);
            ImGui::Text("Ticks per Second: %.1f", currentAnim.ticksPerSecond);
            ImGui::Text("Bone Channels: %zu", currentAnim.boneAnimations.size());

            if (ImGui::TreeNode("Bone List##anim"))
            {
                for (const auto &[boneName, boneInfo] : animComp->boneInfoMap)
                {
                    ImGui::BulletText("%s (ID: %d)", boneName.c_str(), boneInfo.id);
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Channel Info##anim"))
            {
                for (size_t i = 0; i < currentAnim.boneAnimations.size(); i++)
                {
                    const auto &boneAnim = currentAnim.boneAnimations[i];
                    if (ImGui::TreeNode((void *)(intptr_t)i, "%s", boneAnim.boneName.c_str()))
                    {
                        ImGui::Text("Position Keys: %zu", boneAnim.positions.size());
                        ImGui::Text("Rotation Keys: %zu", boneAnim.rotations.size());
                        ImGui::Text("Scale Keys: %zu", boneAnim.scales.size());
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }

    void EditorTimelinePanel::RenderEntityTracks()
    {
        ImGui::SeparatorText("Property Animation Tracks");

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