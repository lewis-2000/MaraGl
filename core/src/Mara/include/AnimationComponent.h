#pragma once
#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <imgui.h>

namespace MaraGl
{
    // Bone information
    struct BoneInfo
    {
        int id;           // Index in final bone transforms array
        glm::mat4 offset; // Transform from mesh space to bone space
    };

    // Keyframe for position
    struct PositionKey
    {
        glm::vec3 position;
        float timeStamp;
    };

    // Keyframe for rotation
    struct RotationKey
    {
        glm::quat orientation;
        float timeStamp;
    };

    // Keyframe for scale
    struct ScaleKey
    {
        glm::vec3 scale;
        float timeStamp;
    };

    // Animation channel for a single bone
    struct BoneAnimation
    {
        std::string boneName;
        std::vector<PositionKey> positions;
        std::vector<RotationKey> rotations;
        std::vector<ScaleKey> scales;
    };

    // Complete animation clip
    struct Animation
    {
        std::string name;
        float duration;       // In ticks
        float ticksPerSecond; // Animation speed
        std::vector<BoneAnimation> boneAnimations;
    };

    struct AnimationComponent : public Component
    {
        struct AnimationLibraryEntry
        {
            std::string displayName;
            std::string sourceModelPath;
            int sourceClipIndex = 0;
            float durationSeconds = 0.0f;
            std::vector<std::string> channelBoneNames;
            glm::vec3 rootTranslationDelta = glm::vec3(0.0f);
            glm::vec3 rootRotationDeltaEuler = glm::vec3(0.0f);
            glm::vec3 rootScaleDelta = glm::vec3(0.0f);
            int resolvedAnimationIndex = -1;
        };

        struct GraphState
        {
            std::string name;
            int libraryClip = -1;
            std::string modelPath; // Legacy fallback for old scene data.
            int clipIndex = 0;     // Legacy fallback for old scene data.
            bool loop = true;
            glm::vec2 nodePosition = glm::vec2(0.0f);
            float durationSeconds = 0.0f;
            glm::vec3 rootTranslationDelta = glm::vec3(0.0f);
            glm::vec3 rootRotationDeltaEuler = glm::vec3(0.0f);
            glm::vec3 rootScaleDelta = glm::vec3(0.0f);
            bool rootMotionEnabled = true;
            bool rootMotionAllowVertical = false;
            float rootMotionScale = 1.0f;
            float rootMotionMaxSpeed = 4.0f;
        };

        struct GraphTransition
        {
            int fromState = 0;
            int toState = 0;
            std::string trigger;
        };

        struct InputBinding
        {
            std::string trigger;
            int key = -1;
        };

        std::vector<Animation> animations;
        std::map<std::string, BoneInfo> boneInfoMap;
        std::vector<glm::mat4> boneTransforms; // Final transforms sent to shader

        // Animation graph authoring/runtime data.
        bool graphEnabled = false;
        int activeState = 0;
        glm::vec2 entryNodePosition = glm::vec2(-280.0f, 60.0f);
        std::vector<AnimationLibraryEntry> animationLibrary;
        std::vector<GraphState> graphStates;
        std::vector<GraphTransition> graphTransitions;
        std::vector<InputBinding> inputBindings;

        int currentAnimation = 0;
        float currentTime = 0.0f;
        bool playing = false;
        bool looping = true;
        float playbackSpeed = 1.0f;

        void OnImGuiRender() override
        {
            ImGui::Text("Animations: %zu", animations.size());
            ImGui::Text("Graph States: %zu", graphStates.size());

            if (animations.empty())
            {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.0f), "No animations loaded");
                return;
            }

            // Animation selector
            if (ImGui::BeginCombo("Animation##anim", animations[currentAnimation].name.c_str()))
            {
                for (size_t i = 0; i < animations.size(); i++)
                {
                    bool isSelected = (currentAnimation == static_cast<int>(i));
                    if (ImGui::Selectable(animations[i].name.c_str(), isSelected))
                    {
                        currentAnimation = static_cast<int>(i);
                        currentTime = 0.0f; // Reset time when switching animations
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // Playback controls
            if (ImGui::Button(playing ? "⏸ Pause##anim" : "▶ Play##anim"))
                playing = !playing;

            ImGui::SameLine();
            if (ImGui::Button("⏹ Stop##anim"))
            {
                playing = false;
                currentTime = 0.0f;
            }

            ImGui::Checkbox("Loop##anim", &looping);

            // Time display and scrubber
            float duration = animations[currentAnimation].duration /
                             animations[currentAnimation].ticksPerSecond;
            ImGui::Text("Time: %.2fs / %.2fs", currentTime, duration);

            if (ImGui::SliderFloat("##timescrub", &currentTime, 0.0f, duration))
            {
                // User manually scrubbed - pause playback
                playing = false;
            }

            ImGui::SliderFloat("Speed##anim", &playbackSpeed, 0.1f, 3.0f);

            // Animation info
            ImGui::Separator();
            ImGui::Text("Duration: %.2fs", duration);
            ImGui::Text("Ticks/Second: %.1f", animations[currentAnimation].ticksPerSecond);
            ImGui::Text("Bones: %zu", boneInfoMap.size());
            ImGui::Text("Channels: %zu", animations[currentAnimation].boneAnimations.size());
        }
    };
}
