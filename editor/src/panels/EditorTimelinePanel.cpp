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
#include <iostream>

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
        RenderBoneSequencer();
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

    void EditorTimelinePanel::DiscoverBonesInAnimation(uint32_t entityID, int animationIndex)
    {
        Entity *entity = m_Scene->FindEntityByID(entityID);
        if (!entity)
            return;

        auto *animComp = entity->GetComponent<AnimationComponent>();
        if (!animComp)
            return;

        if (animationIndex < 0 || animationIndex >= (int)animComp->animations.size())
            return;

        auto &sequencer = m_BoneSequencers[entityID];
        sequencer.entityID = entityID;
        sequencer.animationIndex = animationIndex;
        sequencer.discoveredBones.clear();

        // Get all bone names from the current animation
        const Animation &anim = animComp->animations[animationIndex];
        for (const auto &boneAnim : anim.boneAnimations)
        {
            sequencer.discoveredBones.insert(boneAnim.boneName);
        }
        RefreshBoneSequencerTracks(sequencer, animComp);
    }

    void EditorTimelinePanel::RefreshBoneSequencerTracks(BoneSequencerState &sequencer, AnimationComponent *animComp)
    {
        if (sequencer.animationIndex < 0 || sequencer.animationIndex >= (int)animComp->animations.size())
            return;

        sequencer.tracks.clear();
        const Animation &anim = animComp->animations[sequencer.animationIndex];

        // Create a track for each discovered bone
        for (const auto &boneName : sequencer.discoveredBones)
        {
            // Find the bone animation for this bone
            const BoneAnimation *boneAnim = nullptr;
            for (const auto &ba : anim.boneAnimations)
            {
                if (ba.boneName == boneName)
                {
                    boneAnim = &ba;
                    break;
                }
            }

            if (boneAnim)
            {
                BoneSequencerTrack track;
                track.boneName = boneName;
                track.visible = true;
                track.keyframeCount = boneAnim->positions.size() + boneAnim->rotations.size() + boneAnim->scales.size();

                // Calculate time range
                if (!boneAnim->positions.empty())
                {
                    track.minTime = std::min(track.minTime, boneAnim->positions.front().timeStamp);
                    track.maxTime = std::max(track.maxTime, boneAnim->positions.back().timeStamp);
                }
                if (!boneAnim->rotations.empty())
                {
                    track.minTime = std::min(track.minTime, boneAnim->rotations.front().timeStamp);
                    track.maxTime = std::max(track.maxTime, boneAnim->rotations.back().timeStamp);
                }
                if (!boneAnim->scales.empty())
                {
                    track.minTime = std::min(track.minTime, boneAnim->scales.front().timeStamp);
                    track.maxTime = std::max(track.maxTime, boneAnim->scales.back().timeStamp);
                }

                sequencer.tracks.push_back(track);
            }
        }

        sequencer.needsRefresh = false;
    }

    void EditorTimelinePanel::RenderBoneSequencer()
    {
        ImGui::SeparatorText("Bone Sequencer");

        if (m_SelectedEntityID == 0)
        {
            ImGui::Text("Select an entity to view bone tracks");
            return;
        }

        Entity *entity = m_Scene->FindEntityByID(m_SelectedEntityID);
        if (!entity)
        {
            ImGui::Text("Selected entity not found");
            return;
        }

        auto *animComp = entity->GetComponent<AnimationComponent>();
        if (!animComp || animComp->animations.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No animations loaded");
            return;
        }

        auto &sequencer = m_BoneSequencers[m_SelectedEntityID];

        // If animation changed, rediscover bones
        if (sequencer.animationIndex != animComp->currentAnimation)
        {
            DiscoverBonesInAnimation(m_SelectedEntityID, animComp->currentAnimation);
        }

        if (sequencer.tracks.empty())
        {
            ImGui::Text("No bones found in animation");
            return;
        }

        // Track controls
        ImGui::BeginGroup();
        {
            ImGui::SliderFloat("Zoom##sequencer", &m_TimelinePixelsPerSecond, 10.0f, 500.0f, "%.0f px/sec");
            ImGui::SameLine();
            if (ImGui::Button("Reset Zoom"))
                m_TimelinePixelsPerSecond = 100.0f;

            const Animation &currentAnim = animComp->animations[sequencer.animationIndex];
            float animDuration = currentAnim.duration / currentAnim.ticksPerSecond;
            ImGui::SameLine();
            ImGui::Text("Duration: %.2fs", animDuration);
        }
        ImGui::EndGroup();

        ImGui::Text("Discovered Bones: %zu | Active: %zu",
                    sequencer.tracks.size(),
                    std::count_if(sequencer.tracks.begin(), sequencer.tracks.end(),
                                  [](const BoneSequencerTrack &t)
                                  { return t.visible; }));

        ImGui::Separator();

        // Render track list with sequencer
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        float timelineWidth = ImGui::GetContentRegionAvail().x;
        const Animation &currentAnim = animComp->animations[sequencer.animationIndex];

        // Calculate max time from animation
        float maxAnimTime = currentAnim.duration / currentAnim.ticksPerSecond;

        ImGui::BeginChild("BoneSequencerTracks", ImVec2(timelineWidth, 300.0f), true);
        {
            for (size_t i = 0; i < sequencer.tracks.size(); i++)
            {
                auto &track = sequencer.tracks[i];
                RenderBoneTrack(track, currentAnim, ImGui::GetCursorScreenPos().x, timelineWidth - 20);
            }

            // Draw playback head
            if (animComp->playing || animComp->currentTime > 0.0f)
            {
                float displayTime = animComp->currentTime / currentAnim.ticksPerSecond;
                float animDuration = currentAnim.duration / currentAnim.ticksPerSecond;
                float headPixelX = ImGui::GetWindowPos().x + 80 + (displayTime / animDuration) * (timelineWidth - 100);
                ImVec2 headTop = ImVec2(headPixelX, ImGui::GetWindowPos().y);
                ImVec2 headBottom = ImVec2(headPixelX, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y);
                ImU32 headColor = ImGui::GetColorU32(ImVec4(1.0f, 0.8f, 0.2f, 0.8f));
                drawList->AddLine(headTop, headBottom, headColor, 2.0f);
            }
        }
        ImGui::EndChild();

        // Track visibility controls
        ImGui::SeparatorText("Track Configuration");
        {
            // Toggle all
            bool allVisible = true;
            for (const auto &track : sequencer.tracks)
            {
                if (!track.visible)
                {
                    allVisible = false;
                    break;
                }
            }

            if (ImGui::Button(allVisible ? "Hide All##bones" : "Show All##bones"))
            {
                for (auto &track : sequencer.tracks)
                    track.visible = !allVisible;
            }

            ImGui::SameLine();
            ImGui::Text("Total Bones: %zu | Visible: %zu",
                        sequencer.tracks.size(),
                        std::count_if(sequencer.tracks.begin(), sequencer.tracks.end(),
                                      [](const BoneSequencerTrack &t)
                                      { return t.visible; }));
        }

        ImGui::Separator();

        // Individual track toggles in a table
        if (ImGui::BeginTable("BoneTracks", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 200)))
        {
            ImGui::TableSetupColumn("vis", ImGuiTableColumnFlags_WidthFixed, 40);
            ImGui::TableSetupColumn("Bone Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Keys", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("Range", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            for (auto &track : sequencer.tracks)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Checkbox(("##vis_" + track.boneName).c_str(), &track.visible);

                ImGui::TableSetColumnIndex(1);
                ImGui::Selectable(track.boneName.c_str(), &track.selected, ImGuiSelectableFlags_SpanAllColumns);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Position / Rotation / Scale keys");
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", track.keyframeCount);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.1f-%.1f", track.minTime, track.maxTime);
            }
            ImGui::EndTable();
        }
    }

    void EditorTimelinePanel::RenderBoneTrack(BoneSequencerTrack &track, const Animation &animation, float timelineStartX, float timelineWidth)
    {
        if (!track.visible)
            return;

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float trackHeight = 24.0f;
        float trackY = cursorPos.y;

        // Track background
        ImU32 bgColor = track.selected ? ImGui::GetColorU32(ImVec4(0.3f, 0.4f, 0.6f, 0.5f))
                                       : ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        drawList->AddRectFilled(ImVec2(timelineStartX, trackY), ImVec2(timelineStartX + timelineWidth, trackY + trackHeight), bgColor);

        // Draw bone name label
        ImGui::SetCursorScreenPos(ImVec2(timelineStartX + 5, trackY + 4));
        ImGui::Text("%s", track.boneName.c_str());

        // Find bone animation data
        const BoneAnimation *boneAnim = nullptr;
        for (const auto &ba : animation.boneAnimations)
        {
            if (ba.boneName == track.boneName)
            {
                boneAnim = &ba;
                break;
            }
        }

        if (boneAnim)
        {
            float animDuration = animation.duration / animation.ticksPerSecond;
            float pixelsPerSecond = m_TimelinePixelsPerSecond;

            // Draw position keyframes (red)
            for (const auto &key : boneAnim->positions)
            {
                float keyTime = key.timeStamp / animation.ticksPerSecond;
                float pixelX = timelineStartX + timelineWidth * 0.1f + (keyTime / animDuration) * (timelineWidth * 0.8f);
                ImU32 posColor = ImGui::GetColorU32(ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                drawList->AddCircleFilled(ImVec2(pixelX, trackY + 12), 3.0f, posColor);
            }

            // Draw rotation keyframes (green)
            for (const auto &key : boneAnim->rotations)
            {
                float keyTime = key.timeStamp / animation.ticksPerSecond;
                float pixelX = timelineStartX + timelineWidth * 0.1f + (keyTime / animDuration) * (timelineWidth * 0.8f);
                ImU32 rotColor = ImGui::GetColorU32(ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                drawList->AddCircleFilled(ImVec2(pixelX, trackY + 12), 3.0f, rotColor);
            }

            // Draw scale keyframes (blue)
            for (const auto &key : boneAnim->scales)
            {
                float keyTime = key.timeStamp / animation.ticksPerSecond;
                float pixelX = timelineStartX + timelineWidth * 0.1f + (keyTime / animDuration) * (timelineWidth * 0.8f);
                ImU32 scaleColor = ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 1.0f, 1.0f));
                drawList->AddCircleFilled(ImVec2(pixelX, trackY + 12), 3.0f, scaleColor);
            }
        }

        // Track outline
        ImU32 outlineColor = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        drawList->AddRect(ImVec2(timelineStartX, trackY), ImVec2(timelineStartX + timelineWidth, trackY + trackHeight), outlineColor);

        ImGui::Dummy(ImVec2(timelineWidth, trackHeight));
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