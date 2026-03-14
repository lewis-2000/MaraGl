#include "AnimationGraphPanel.h"
#include "Model.h"
#include <imnodes.h>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <cctype>
#include <cmath>
#include <limits>
#include <future>
#include <chrono>

namespace MaraGl
{
    namespace
    {
        struct KeyOption
        {
            const char *label;
            int key;
        };

        constexpr KeyOption kKeyOptions[] = {
            {"None", GLFW_KEY_UNKNOWN},
            {"1", GLFW_KEY_1},
            {"2", GLFW_KEY_2},
            {"3", GLFW_KEY_3},
            {"4", GLFW_KEY_4},
            {"5", GLFW_KEY_5},
            {"6", GLFW_KEY_6},
            {"7", GLFW_KEY_7},
            {"8", GLFW_KEY_8},
            {"9", GLFW_KEY_9},
            {"W", GLFW_KEY_W},
            {"A", GLFW_KEY_A},
            {"S", GLFW_KEY_S},
            {"D", GLFW_KEY_D},
            {"Space", GLFW_KEY_SPACE},
            {"Left Shift", GLFW_KEY_LEFT_SHIFT},
            {"E", GLFW_KEY_E},
            {"R", GLFW_KEY_R},
            {"Q", GLFW_KEY_Q},
            {"T", GLFW_KEY_T}};

        constexpr int kNodeIdBase = 10000;
        constexpr int kAttrIdBase = 20000;
        constexpr int kLinkIdBase = 30000;
        constexpr int kEntryNodeId = 9000;
        constexpr int kEntryOutputAttrId = 19000;
        constexpr int kEntryLinkId = 39000;

        int NodeIdFromIndex(size_t index)
        {
            return kNodeIdBase + static_cast<int>(index);
        }

        int InputAttrIdFromNodeIndex(size_t nodeIndex)
        {
            return kAttrIdBase + static_cast<int>(nodeIndex * 2);
        }

        int OutputAttrIdFromNodeIndex(size_t nodeIndex)
        {
            return kAttrIdBase + static_cast<int>(nodeIndex * 2 + 1);
        }

        int LinkIdFromIndex(size_t index)
        {
            return kLinkIdBase + static_cast<int>(index);
        }

        std::string ToLowerAscii(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c)
                           { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        bool IsGenericMixamoName(const std::string &name)
        {
            const std::string lowered = ToLowerAscii(name);
            return lowered.empty() || lowered == "mixamo.com" || lowered == "mixamo";
        }

        std::string NormalizeClipDisplayName(const std::string &raw)
        {
            std::string spaced;
            spaced.reserve(raw.size() * 2);

            char prev = '\0';
            for (char c : raw)
            {
                const unsigned char uc = static_cast<unsigned char>(c);
                const bool isSep = (c == '_' || c == '-' || c == '.');
                if (isSep)
                {
                    if (!spaced.empty() && spaced.back() != ' ')
                        spaced.push_back(' ');
                    prev = c;
                    continue;
                }

                if (std::isupper(uc) && std::islower(static_cast<unsigned char>(prev)) && !spaced.empty() && spaced.back() != ' ')
                    spaced.push_back(' ');

                spaced.push_back(c);
                prev = c;
            }

            std::string collapsed;
            collapsed.reserve(spaced.size());
            bool lastWasSpace = true;
            for (char c : spaced)
            {
                const bool isSpace = std::isspace(static_cast<unsigned char>(c)) != 0;
                if (isSpace)
                {
                    if (!lastWasSpace)
                        collapsed.push_back(' ');
                    lastWasSpace = true;
                }
                else
                {
                    collapsed.push_back(c);
                    lastWasSpace = false;
                }
            }

            while (!collapsed.empty() && collapsed.back() == ' ')
                collapsed.pop_back();

            bool startOfWord = true;
            for (char &c : collapsed)
            {
                unsigned char uc = static_cast<unsigned char>(c);
                if (std::isspace(uc))
                {
                    startOfWord = true;
                    continue;
                }
                c = startOfWord ? static_cast<char>(std::toupper(uc)) : static_cast<char>(std::tolower(uc));
                startOfWord = false;
            }

            return collapsed;
        }

        std::string BuildClipDisplayName(const Animation &clip,
                                         const std::string &sourcePath,
                                         int clipIndex,
                                         size_t clipCount,
                                         const std::vector<AnimationComponent::AnimationLibraryEntry> &existingEntries)
        {
            const std::string sourceStem = std::filesystem::path(sourcePath).stem().string();
            std::string baseName = clip.name;

            if (IsGenericMixamoName(baseName))
                baseName = sourceStem;

            if (baseName.empty())
                baseName = "Clip";

            baseName = NormalizeClipDisplayName(baseName);

            if (clipCount > 1)
                baseName += " " + std::to_string(clipIndex + 1);

            std::string uniqueName = baseName;
            int suffix = 2;
            auto hasName = [&](const std::string &candidate)
            {
                for (const auto &entry : existingEntries)
                {
                    if (entry.displayName == candidate)
                        return true;
                }
                return false;
            };

            while (hasName(uniqueName))
                uniqueName = baseName + "_" + std::to_string(suffix++);

            return uniqueName;
        }

        bool TryStateIndexFromInputAttrId(int attrId, int &outStateIndex)
        {
            if (attrId < kAttrIdBase)
                return false;

            const int local = attrId - kAttrIdBase;
            if ((local % 2) != 0)
                return false;

            outStateIndex = local / 2;
            return true;
        }

        bool TryStateIndexFromOutputAttrId(int attrId, int &outStateIndex)
        {
            if (attrId < kAttrIdBase)
                return false;

            const int local = attrId - kAttrIdBase;
            if ((local % 2) != 1)
                return false;

            outStateIndex = local / 2;
            return true;
        }

        bool TryTransitionIndexFromLinkId(int linkId, int &outTransitionIndex)
        {
            if (linkId >= kLinkIdBase && linkId < kEntryLinkId)
            {
                outTransitionIndex = linkId - kLinkIdBase;
                return true;
            }
            return false;
        }

        std::string ToTriggerName(const std::string &name)
        {
            std::string out;
            out.reserve(name.size());
            for (char c : name)
            {
                if (std::isalnum(static_cast<unsigned char>(c)))
                    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
                else if (!out.empty() && out.back() != '_')
                    out.push_back('_');
            }
            if (out.empty())
                out = "trigger";
            return out;
        }

        void ExtractRootTransformData(const Animation &clip,
                                      glm::vec3 &translationDelta,
                                      glm::vec3 &rotationDeltaEuler,
                                      glm::vec3 &scaleDelta)
        {
            translationDelta = glm::vec3(0.0f);
            rotationDeltaEuler = glm::vec3(0.0f);
            scaleDelta = glm::vec3(0.0f);

            const BoneAnimation *channel = nullptr;
            for (const auto &bone : clip.boneAnimations)
            {
                if (!bone.positions.empty() || !bone.rotations.empty() || !bone.scales.empty())
                {
                    channel = &bone;
                    if (bone.boneName.find("Hips") != std::string::npos ||
                        bone.boneName.find("Root") != std::string::npos ||
                        bone.boneName.find("root") != std::string::npos)
                        break;
                }
            }

            if (!channel)
                return;

            if (channel->positions.size() >= 2)
            {
                translationDelta = channel->positions.back().position - channel->positions.front().position;
            }

            if (channel->rotations.size() >= 2)
            {
                const glm::quat q0 = channel->rotations.front().orientation;
                const glm::quat q1 = channel->rotations.back().orientation;
                const glm::quat qDelta = q1 * glm::inverse(q0);
                rotationDeltaEuler = glm::degrees(glm::eulerAngles(qDelta));
            }

            if (channel->scales.size() >= 2)
            {
                scaleDelta = channel->scales.back().scale - channel->scales.front().scale;
            }
        }

        int FindLibraryEntryIndex(const AnimationComponent *anim, const std::string &sourceModelPath, int sourceClipIndex)
        {
            if (!anim)
                return -1;

            for (size_t i = 0; i < anim->animationLibrary.size(); ++i)
            {
                const auto &entry = anim->animationLibrary[i];
                if (entry.sourceModelPath == sourceModelPath && entry.sourceClipIndex == sourceClipIndex)
                    return static_cast<int>(i);
            }
            return -1;
        }

        const AnimationComponent::AnimationLibraryEntry *GetLibraryEntry(const AnimationComponent *anim, int libraryClip)
        {
            if (!anim || libraryClip < 0 || libraryClip >= static_cast<int>(anim->animationLibrary.size()))
                return nullptr;
            return &anim->animationLibrary[static_cast<size_t>(libraryClip)];
        }

        int CountMatchingChannelBones(const AnimationComponent *anim, const AnimationComponent::AnimationLibraryEntry &entry)
        {
            if (!anim)
                return 0;

            int matchCount = 0;
            for (const auto &boneName : entry.channelBoneNames)
            {
                if (anim->boneInfoMap.find(boneName) != anim->boneInfoMap.end())
                    ++matchCount;
            }
            return matchCount;
        }

        const char *CompatibilityLabel(const AnimationComponent *anim, const AnimationComponent::AnimationLibraryEntry &entry)
        {
            if (anim->boneInfoMap.empty() || entry.channelBoneNames.empty())
                return "Unknown";

            const int matchCount = CountMatchingChannelBones(anim, entry);
            if (matchCount == 0)
                return "Mismatch";
            if (matchCount == static_cast<int>(entry.channelBoneNames.size()))
                return "Compatible";
            return "Partial";
        }

        ImVec4 CompatibilityColor(const char *label)
        {
            if (std::strcmp(label, "Compatible") == 0)
                return ImVec4(0.25f, 0.78f, 0.45f, 1.0f);
            if (std::strcmp(label, "Partial") == 0)
                return ImVec4(0.90f, 0.72f, 0.20f, 1.0f);
            if (std::strcmp(label, "Mismatch") == 0)
                return ImVec4(0.88f, 0.32f, 0.32f, 1.0f);
            return ImVec4(0.55f, 0.55f, 0.60f, 1.0f);
        }

        struct GraphValidationSummary
        {
            int errors = 0;
            int warnings = 0;
        };

        GraphValidationSummary ValidateGraph(const AnimationComponent *anim)
        {
            GraphValidationSummary summary;
            if (!anim)
                return summary;

            const int stateCount = static_cast<int>(anim->graphStates.size());
            if (stateCount == 0)
            {
                if (anim->graphEnabled)
                    summary.warnings++;
                return summary;
            }

            if (anim->activeState < 0 || anim->activeState >= stateCount)
                summary.errors++;

            for (const auto &transition : anim->graphTransitions)
            {
                if (transition.fromState < 0 || transition.fromState >= stateCount ||
                    transition.toState < 0 || transition.toState >= stateCount)
                {
                    summary.errors++;
                }
            }

            std::vector<bool> reachable(static_cast<size_t>(stateCount), false);
            if (anim->activeState >= 0 && anim->activeState < stateCount)
            {
                std::vector<int> frontier = {anim->activeState};
                reachable[static_cast<size_t>(anim->activeState)] = true;
                for (size_t cursor = 0; cursor < frontier.size(); ++cursor)
                {
                    const int current = frontier[cursor];
                    for (const auto &transition : anim->graphTransitions)
                    {
                        if (transition.fromState != current)
                            continue;
                        if (transition.toState < 0 || transition.toState >= stateCount)
                            continue;
                        if (!reachable[static_cast<size_t>(transition.toState)])
                        {
                            reachable[static_cast<size_t>(transition.toState)] = true;
                            frontier.push_back(transition.toState);
                        }
                    }
                }
            }

            for (int i = 0; i < stateCount; ++i)
            {
                const auto &state = anim->graphStates[static_cast<size_t>(i)];
                if (state.libraryClip < 0)
                    summary.warnings++;
                if (!reachable[static_cast<size_t>(i)])
                    summary.warnings++;
            }

            return summary;
        }

        AnimationComponent::GraphState CreateBlankState(size_t stateIndex)
        {
            AnimationComponent::GraphState state;
            state.name = "State" + std::to_string(stateIndex);
            state.libraryClip = -1;
            state.modelPath.clear();
            state.clipIndex = 0;
            state.loop = true;
            state.nodePosition = glm::vec2(0.0f);
            state.durationSeconds = 0.0f;
            state.rootTranslationDelta = glm::vec3(0.0f);
            state.rootRotationDeltaEuler = glm::vec3(0.0f);
            state.rootScaleDelta = glm::vec3(0.0f);
            return state;
        }

        std::string MakeUniqueDisplayName(const AnimationComponent *anim, const std::string &baseName)
        {
            std::string uniqueName = baseName.empty() ? "Clip" : baseName;
            int suffix = 2;
            auto hasName = [&](const std::string &candidate)
            {
                if (!anim)
                    return false;
                for (const auto &entry : anim->animationLibrary)
                {
                    if (entry.displayName == candidate)
                        return true;
                }
                return false;
            };

            while (hasName(uniqueName))
                uniqueName = baseName + "_" + std::to_string(suffix++);

            return uniqueName;
        }

        std::vector<AnimationComponent::AnimationLibraryEntry> BuildLibraryEntriesFromFiles(const std::vector<std::string> &files)
        {
            std::vector<AnimationComponent::AnimationLibraryEntry> importedEntries;
            for (const auto &path : files)
            {
                try
                {
                    Model sourceModel(path);
                    if (!sourceModel.HasAnimations())
                        continue;

                    const auto clips = sourceModel.LoadAnimations();
                    for (size_t i = 0; i < clips.size(); ++i)
                    {
                        const auto &clip = clips[i];
                        AnimationComponent::AnimationLibraryEntry entry;
                        entry.displayName = BuildClipDisplayName(clip,
                                                                 path,
                                                                 static_cast<int>(i),
                                                                 clips.size(),
                                                                 importedEntries);
                        entry.sourceModelPath = path;
                        entry.sourceClipIndex = static_cast<int>(i);
                        entry.durationSeconds = (clip.ticksPerSecond > 0.0f) ? (clip.duration / clip.ticksPerSecond) : 0.0f;
                        for (const auto &boneAnim : clip.boneAnimations)
                            entry.channelBoneNames.push_back(boneAnim.boneName);
                        ExtractRootTransformData(clip, entry.rootTranslationDelta, entry.rootRotationDeltaEuler, entry.rootScaleDelta);
                        importedEntries.push_back(std::move(entry));
                    }
                }
                catch (const std::exception &)
                {
                    // Keep importing remaining files if one file fails.
                }
            }
            return importedEntries;
        }

        void RemoveStateAndRepairGraph(AnimationComponent *anim, int stateIndex)
        {
            if (!anim || stateIndex < 0 || stateIndex >= static_cast<int>(anim->graphStates.size()))
                return;

            anim->graphStates.erase(anim->graphStates.begin() + stateIndex);

            anim->graphTransitions.erase(
                std::remove_if(anim->graphTransitions.begin(), anim->graphTransitions.end(),
                               [stateIndex](const AnimationComponent::GraphTransition &transition)
                               {
                                   return transition.fromState == stateIndex || transition.toState == stateIndex;
                               }),
                anim->graphTransitions.end());

            for (auto &transition : anim->graphTransitions)
            {
                if (transition.fromState > stateIndex)
                    --transition.fromState;
                if (transition.toState > stateIndex)
                    --transition.toState;
            }

            if (anim->graphStates.empty())
            {
                anim->activeState = 0;
            }
            else if (anim->activeState == stateIndex)
            {
                anim->activeState = std::clamp(stateIndex - 1, 0, static_cast<int>(anim->graphStates.size()) - 1);
            }
            else if (anim->activeState > stateIndex)
            {
                --anim->activeState;
            }
        }
    }

    const char *AnimationGraphPanel::KeyToLabel(int key)
    {
        for (const auto &opt : kKeyOptions)
        {
            if (opt.key == key)
                return opt.label;
        }
        return "Unknown";
    }

    void AnimationGraphPanel::RefreshAnimationFiles()
    {
        m_AnimationFiles.clear();
        const std::filesystem::path dir("resources/Animations");
        if (!std::filesystem::exists(dir))
            return;

        for (const auto &entry : std::filesystem::directory_iterator(dir))
        {
            if (!entry.is_regular_file())
                continue;
            const auto ext = entry.path().extension().string();
            if (ext == ".fbx" || ext == ".FBX")
                m_AnimationFiles.push_back(entry.path().generic_string());
        }
        std::sort(m_AnimationFiles.begin(), m_AnimationFiles.end());
        if (m_SelectedImportFile >= static_cast<int>(m_AnimationFiles.size()))
            m_SelectedImportFile = 0;
        m_FilesScanned = true;
    }

    void AnimationGraphPanel::ExtractGraphFromFiles(AnimationComponent *anim, const std::vector<std::string> &files, bool replaceExisting)
    {
        if (!anim)
            return;

        if (replaceExisting)
        {
            anim->animationLibrary.clear();
        }

        for (const auto &path : files)
        {
            try
            {
                Model sourceModel(path);
                if (!sourceModel.HasAnimations())
                    continue;

                const auto clips = sourceModel.LoadAnimations();
                for (size_t i = 0; i < clips.size(); ++i)
                {
                    const auto &clip = clips[i];
                    if (FindLibraryEntryIndex(anim, path, static_cast<int>(i)) == -1)
                    {
                        AnimationComponent::AnimationLibraryEntry entry;
                        entry.displayName = BuildClipDisplayName(clip,
                                                                 path,
                                                                 static_cast<int>(i),
                                                                 clips.size(),
                                                                 anim->animationLibrary);
                        entry.sourceModelPath = path;
                        entry.sourceClipIndex = static_cast<int>(i);
                        entry.durationSeconds = (clip.ticksPerSecond > 0.0f) ? (clip.duration / clip.ticksPerSecond) : 0.0f;
                        for (const auto &boneAnim : clip.boneAnimations)
                            entry.channelBoneNames.push_back(boneAnim.boneName);
                        ExtractRootTransformData(clip, entry.rootTranslationDelta, entry.rootRotationDeltaEuler, entry.rootScaleDelta);
                        anim->animationLibrary.push_back(entry);
                    }
                }
            }
            catch (const std::exception &)
            {
                // Keep the import resilient and continue with remaining files.
            }
        }
    }

    void AnimationGraphPanel::DrawGraphCanvas(AnimationComponent *anim)
    {
        ImNodesIO &nodesIO = ImNodes::GetIO();
        nodesIO.EmulateThreeButtonMouse.Modifier = &ImGui::GetIO().KeyAlt;
        nodesIO.AltMouseButton = ImGuiMouseButton_Left;
        const ImVec2 currentPanning = ImNodes::EditorContextGetPanning();

        if (m_RequestFocusNodeId != -1)
        {
            ImNodes::EditorContextMoveToNode(m_RequestFocusNodeId);
            m_RequestFocusNodeId = -1;
        }

        ImGui::TextUnformatted("Animation State Graph");
        ImGui::TextDisabled("Pan: Middle-mouse drag or Alt+Left drag (trackpad)");
        ImGui::Separator();
        const ImVec2 editorCanvasSize = ImGui::GetContentRegionAvail();

        ImNodes::BeginNodeEditor();

        if (m_RequestAutoLayout)
        {
            anim->entryNodePosition = glm::vec2(-280.0f, 60.0f);
            ImNodes::SetNodeGridSpacePos(kEntryNodeId, ImVec2(anim->entryNodePosition.x, anim->entryNodePosition.y));
            const int gridSize = 3;
            const float horizontalGap = 280.0f;
            const float verticalGap = 190.0f;
            for (size_t i = 0; i < anim->graphStates.size(); ++i)
            {
                const int row = static_cast<int>(i) / gridSize;
                const int col = static_cast<int>(i) % gridSize;
                anim->graphStates[i].nodePosition = glm::vec2(col * horizontalGap, row * verticalGap);
                ImNodes::SetNodeGridSpacePos(NodeIdFromIndex(i), ImVec2(anim->graphStates[i].nodePosition.x, anim->graphStates[i].nodePosition.y));
            }
            m_LastLaidOutStateCount = anim->graphStates.size();
            m_RequestAutoLayout = false;
            m_RequestRestorePositions = false;
            ImNodes::EditorContextResetPanning(ImVec2(220.0f, 140.0f));
        }
        else if (m_RequestRestorePositions)
        {
            ImNodes::SetNodeGridSpacePos(kEntryNodeId, ImVec2(anim->entryNodePosition.x, anim->entryNodePosition.y));
            for (size_t i = 0; i < anim->graphStates.size(); ++i)
            {
                ImNodes::SetNodeGridSpacePos(NodeIdFromIndex(i), ImVec2(anim->graphStates[i].nodePosition.x, anim->graphStates[i].nodePosition.y));
            }
            m_LastLaidOutStateCount = anim->graphStates.size();
            m_RequestRestorePositions = false;
        }
        else if (anim->graphStates.size() > m_LastLaidOutStateCount)
        {
            const size_t i = m_LastLaidOutStateCount;
            const int gridSize = 3;
            const float horizontalGap = 280.0f;
            const float verticalGap = 190.0f;
            const int row = static_cast<int>(i) / gridSize;
            const int col = static_cast<int>(i) % gridSize;
            anim->graphStates[i].nodePosition = glm::vec2(col * horizontalGap, row * verticalGap);
            ImNodes::SetNodeGridSpacePos(NodeIdFromIndex(i), ImVec2(anim->graphStates[i].nodePosition.x, anim->graphStates[i].nodePosition.y));
            m_LastLaidOutStateCount = anim->graphStates.size();
        }

        ImGui::PushID(kEntryNodeId);
        ImNodes::BeginNode(kEntryNodeId);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Entry");
        ImNodes::EndNodeTitleBar();
        ImGui::TextDisabled("Graph start point");
        ImNodes::BeginOutputAttribute(kEntryOutputAttrId, ImNodesPinShape_TriangleFilled);
        ImGui::Button("Start##entry", ImVec2(140.0f, 0.0f));
        ImNodes::EndOutputAttribute();
        ImNodes::EndNode();
        ImGui::PopID();

        for (size_t i = 0; i < anim->graphStates.size(); ++i)
        {
            const int nodeId = NodeIdFromIndex(i);
            const auto &state = anim->graphStates[i];

            ImGui::PushID(nodeId);
            ImNodes::BeginNode(nodeId);

            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(state.name.c_str());
            ImNodes::EndNodeTitleBar();

            const auto *libraryEntry = GetLibraryEntry(anim, state.libraryClip);
            const std::string clipLabel = libraryEntry
                                              ? (libraryEntry->displayName + " [" + std::to_string(libraryEntry->sourceClipIndex) + "]")
                                              : std::string("No clip assigned");
            ImGui::TextDisabled("Clip");
            ImGui::TextWrapped("%s", clipLabel.c_str());
            ImGui::Text("Duration %.2fs", state.durationSeconds);
            ImGui::Text("Loop %s", state.loop ? "On" : "Off");

            ImNodes::BeginInputAttribute(InputAttrIdFromNodeIndex(i), ImNodesPinShape_CircleFilled);
            ImGui::Button("In##state", ImVec2(72.0f, 0.0f));
            ImNodes::EndInputAttribute();

            ImGui::SameLine();

            ImNodes::BeginOutputAttribute(OutputAttrIdFromNodeIndex(i), ImNodesPinShape_CircleFilled);
            ImGui::Button("Out##state", ImVec2(72.0f, 0.0f));
            ImNodes::EndOutputAttribute();

            ImNodes::EndNode();
            ImGui::PopID();
        }

        if (anim->activeState >= 0 && anim->activeState < static_cast<int>(anim->graphStates.size()))
        {
            ImNodes::Link(kEntryLinkId, kEntryOutputAttrId, InputAttrIdFromNodeIndex(static_cast<size_t>(anim->activeState)));
        }

        for (size_t linkIdx = 0; linkIdx < anim->graphTransitions.size(); ++linkIdx)
        {
            const auto &transition = anim->graphTransitions[linkIdx];
            if (transition.fromState >= 0 && transition.fromState < static_cast<int>(anim->graphStates.size()) &&
                transition.toState >= 0 && transition.toState < static_cast<int>(anim->graphStates.size()))
            {
                ImNodes::Link(LinkIdFromIndex(linkIdx),
                              OutputAttrIdFromNodeIndex(static_cast<size_t>(transition.fromState)),
                              InputAttrIdFromNodeIndex(static_cast<size_t>(transition.toState)));
            }
        }

        ImNodes::MiniMap(0.18f, ImNodesMiniMapLocation_TopRight);

        ImNodes::EndNodeEditor();

        if (ImNodes::IsEditorHovered() && ImGui::BeginPopupContextWindow("AnimationGraphCanvasContext"))
        {
            if (ImGui::MenuItem("Add State"))
            {
                AnimationComponent::GraphState state = CreateBlankState(anim->graphStates.size());
                state.nodePosition = glm::vec2(-currentPanning.x + editorCanvasSize.x * 0.5f,
                                               -currentPanning.y + editorCanvasSize.y * 0.5f);
                anim->graphStates.push_back(state);
                m_LastLaidOutStateCount = anim->graphStates.size();
                m_RequestRestorePositions = false;
            }

            const bool hasSelectedNodes = (ImNodes::NumSelectedNodes() > 0);
            const bool hasSelectedLinks = (ImNodes::NumSelectedLinks() > 0);
            if (ImGui::MenuItem("Delete Selected", nullptr, false, hasSelectedNodes || hasSelectedLinks))
            {
                if (hasSelectedLinks)
                {
                    std::vector<int> selectedLinks(static_cast<size_t>(ImNodes::NumSelectedLinks()));
                    ImNodes::GetSelectedLinks(selectedLinks.data());
                    std::sort(selectedLinks.begin(), selectedLinks.end(), std::greater<int>());
                    for (int linkId : selectedLinks)
                    {
                        int transitionIndex = -1;
                        if (TryTransitionIndexFromLinkId(linkId, transitionIndex) &&
                            transitionIndex >= 0 && transitionIndex < static_cast<int>(anim->graphTransitions.size()))
                        {
                            anim->graphTransitions.erase(anim->graphTransitions.begin() + transitionIndex);
                        }
                    }
                    ImNodes::ClearLinkSelection();
                    m_SelectedTransitionIndex = -1;
                }

                if (hasSelectedNodes)
                {
                    std::vector<int> selectedNodes(static_cast<size_t>(ImNodes::NumSelectedNodes()));
                    ImNodes::GetSelectedNodes(selectedNodes.data());
                    std::vector<int> stateIndices;
                    for (int nodeId : selectedNodes)
                    {
                        if (nodeId >= kNodeIdBase)
                            stateIndices.push_back(nodeId - kNodeIdBase);
                    }
                    std::sort(stateIndices.begin(), stateIndices.end(), std::greater<int>());
                    for (int stateIndex : stateIndices)
                        RemoveStateAndRepairGraph(anim, stateIndex);
                    ImNodes::ClearNodeSelection();
                    m_SelectedStateIndex = -1;
                    m_LastLaidOutStateCount = anim->graphStates.size();
                }
            }
            ImGui::EndPopup();
        }

        int createdStartAttr = -1;
        int createdEndAttr = -1;
        if (ImNodes::IsLinkCreated(&createdStartAttr, &createdEndAttr))
        {
            int fromStateIndex = -1;
            int toStateIndex = -1;

            const bool startIsEntry = (createdStartAttr == kEntryOutputAttrId);
            const bool endIsEntry = (createdEndAttr == kEntryOutputAttrId);
            const bool startIsOutput = TryStateIndexFromOutputAttrId(createdStartAttr, fromStateIndex);
            const bool endIsOutput = TryStateIndexFromOutputAttrId(createdEndAttr, fromStateIndex);
            const bool startIsInput = TryStateIndexFromInputAttrId(createdStartAttr, toStateIndex);
            const bool endIsInput = TryStateIndexFromInputAttrId(createdEndAttr, toStateIndex);

            if (startIsEntry && endIsInput)
            {
                anim->activeState = toStateIndex;
            }
            else if (endIsEntry && startIsInput)
            {
                anim->activeState = toStateIndex;
            }
            else
            {
                int linkFromState = -1;
                int linkToState = -1;
                if (TryStateIndexFromOutputAttrId(createdStartAttr, linkFromState) && TryStateIndexFromInputAttrId(createdEndAttr, linkToState))
                {
                }
                else if (TryStateIndexFromOutputAttrId(createdEndAttr, linkFromState) && TryStateIndexFromInputAttrId(createdStartAttr, linkToState))
                {
                }

                if (linkFromState >= 0 && linkToState >= 0 && linkFromState != linkToState)
                {
                    bool duplicate = false;
                    for (const auto &transition : anim->graphTransitions)
                    {
                        if (transition.fromState == linkFromState && transition.toState == linkToState)
                        {
                            duplicate = true;
                            break;
                        }
                    }

                    if (!duplicate)
                    {
                        AnimationComponent::GraphTransition transition;
                        transition.fromState = linkFromState;
                        transition.toState = linkToState;
                        transition.trigger = ToTriggerName(anim->graphStates[static_cast<size_t>(linkToState)].name);
                        anim->graphTransitions.push_back(transition);
                        m_SelectedTransitionIndex = static_cast<int>(anim->graphTransitions.size()) - 1;
                    }
                }
            }
        }

        int destroyedLinkId = -1;
        if (ImNodes::IsLinkDestroyed(&destroyedLinkId))
        {
            int transitionIndex = -1;
            if (TryTransitionIndexFromLinkId(destroyedLinkId, transitionIndex) &&
                transitionIndex >= 0 && transitionIndex < static_cast<int>(anim->graphTransitions.size()))
            {
                anim->graphTransitions.erase(anim->graphTransitions.begin() + transitionIndex);
                m_SelectedTransitionIndex = -1;
            }
        }

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        {
            const int selectedLinkCount = ImNodes::NumSelectedLinks();
            const int selectedNodeCountForDelete = ImNodes::NumSelectedNodes();
            if (selectedLinkCount > 0)
            {
                std::vector<int> selectedLinks(static_cast<size_t>(selectedLinkCount));
                ImNodes::GetSelectedLinks(selectedLinks.data());
                std::sort(selectedLinks.begin(), selectedLinks.end(), std::greater<int>());
                for (int linkId : selectedLinks)
                {
                    int transitionIndex = -1;
                    if (TryTransitionIndexFromLinkId(linkId, transitionIndex) &&
                        transitionIndex >= 0 && transitionIndex < static_cast<int>(anim->graphTransitions.size()))
                    {
                        anim->graphTransitions.erase(anim->graphTransitions.begin() + transitionIndex);
                    }
                }
                ImNodes::ClearLinkSelection();
                m_SelectedTransitionIndex = -1;
            }

            if (selectedNodeCountForDelete > 0)
            {
                std::vector<int> selectedNodes(static_cast<size_t>(selectedNodeCountForDelete));
                ImNodes::GetSelectedNodes(selectedNodes.data());
                std::vector<int> stateIndices;
                for (int nodeId : selectedNodes)
                {
                    if (nodeId >= kNodeIdBase)
                        stateIndices.push_back(nodeId - kNodeIdBase);
                }
                std::sort(stateIndices.begin(), stateIndices.end(), std::greater<int>());
                for (int stateIndex : stateIndices)
                    RemoveStateAndRepairGraph(anim, stateIndex);
                ImNodes::ClearNodeSelection();
                m_SelectedStateIndex = -1;
                m_LastLaidOutStateCount = anim->graphStates.size();
            }
        }

        anim->entryNodePosition = glm::vec2(ImNodes::GetNodeGridSpacePos(kEntryNodeId).x,
                                            ImNodes::GetNodeGridSpacePos(kEntryNodeId).y);
        for (size_t i = 0; i < anim->graphStates.size(); ++i)
        {
            const ImVec2 nodePos = ImNodes::GetNodeGridSpacePos(NodeIdFromIndex(i));
            anim->graphStates[i].nodePosition = glm::vec2(nodePos.x, nodePos.y);
        }

        if (m_RequestFitView)
        {
            ImVec2 boundsMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
            ImVec2 boundsMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

            auto includeNodeBounds = [&](int nodeId)
            {
                const ImVec2 pos = ImNodes::GetNodeGridSpacePos(nodeId);
                ImVec2 dim = ImNodes::GetNodeDimensions(nodeId);
                if (dim.x <= 0.0f)
                    dim.x = 180.0f;
                if (dim.y <= 0.0f)
                    dim.y = 100.0f;

                boundsMin.x = std::min(boundsMin.x, pos.x);
                boundsMin.y = std::min(boundsMin.y, pos.y);
                boundsMax.x = std::max(boundsMax.x, pos.x + dim.x);
                boundsMax.y = std::max(boundsMax.y, pos.y + dim.y);
            };

            includeNodeBounds(kEntryNodeId);
            for (size_t i = 0; i < anim->graphStates.size(); ++i)
                includeNodeBounds(NodeIdFromIndex(i));

            const float margin = 80.0f;
            boundsMin.x -= margin;
            boundsMin.y -= margin;
            boundsMax.x += margin;
            boundsMax.y += margin;

            const ImVec2 boundsCenter((boundsMin.x + boundsMax.x) * 0.5f, (boundsMin.y + boundsMax.y) * 0.5f);
            const ImVec2 targetCenter(editorCanvasSize.x * 0.5f, editorCanvasSize.y * 0.5f);
            const ImVec2 targetPanning(targetCenter.x - boundsCenter.x, targetCenter.y - boundsCenter.y);
            ImNodes::EditorContextResetPanning(targetPanning);

            m_RequestFitView = false;
        }

        m_SelectedStateIndex = -1;
        m_SelectedTransitionIndex = -1;

        const int selectedNodeCount = ImNodes::NumSelectedNodes();
        if (selectedNodeCount > 0)
        {
            std::vector<int> selectedNodes(static_cast<size_t>(selectedNodeCount));
            ImNodes::GetSelectedNodes(selectedNodes.data());
            const int selectedNodeId = selectedNodes.front();
            if (selectedNodeId >= kNodeIdBase)
                m_SelectedStateIndex = selectedNodeId - kNodeIdBase;
        }

        const int selectedLinkCount = ImNodes::NumSelectedLinks();
        if (selectedLinkCount > 0)
        {
            std::vector<int> selectedLinks(static_cast<size_t>(selectedLinkCount));
            ImNodes::GetSelectedLinks(selectedLinks.data());
            int transitionIndex = -1;
            if (TryTransitionIndexFromLinkId(selectedLinks.front(), transitionIndex))
                m_SelectedTransitionIndex = transitionIndex;
        }
    }

    void AnimationGraphPanel::DrawLibraryPane(AnimationComponent *anim)
    {
        ImGui::TextUnformatted("Animation Library");
        ImGui::Separator();
        ImGui::InputTextWithHint("##libraryFilter", "Filter clips...", m_LibraryFilter, sizeof(m_LibraryFilter));

        if (ImGui::Button("Rescan##toolbar"))
            RefreshAnimationFiles();

        if (m_ImportInProgress)
        {
            ImGui::TextDisabled("Importing animation data in background...");
        }
        else if (m_ImportAddedCount > 0)
        {
            ImGui::TextDisabled("Imported %d clips", m_ImportAddedCount);
        }

        if (!m_AnimationFiles.empty())
        {
            const std::string selectedSourceLabel = std::filesystem::path(m_AnimationFiles[static_cast<size_t>(m_SelectedImportFile)]).filename().string();
            if (ImGui::BeginCombo("Import Source", selectedSourceLabel.c_str()))
            {
                for (int i = 0; i < static_cast<int>(m_AnimationFiles.size()); ++i)
                {
                    const bool selected = (m_SelectedImportFile == i);
                    if (ImGui::Selectable(m_AnimationFiles[static_cast<size_t>(i)].c_str(), selected))
                        m_SelectedImportFile = i;
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::BeginDisabled(m_ImportInProgress);
            if (ImGui::Button("Import Selected Source"))
                StartLibraryImport({m_AnimationFiles[static_cast<size_t>(m_SelectedImportFile)]}, m_LastEntityId);
            ImGui::SameLine();
            if (ImGui::Button("Import All Sources"))
                StartLibraryImport(m_AnimationFiles, m_LastEntityId);
            ImGui::EndDisabled();
        }

        ImGui::Separator();
        ImGui::BeginChild("LibraryList", ImVec2(0.0f, 0.0f), false);
        for (size_t i = 0; i < anim->animationLibrary.size(); ++i)
        {
            const auto &entry = anim->animationLibrary[i];
            const std::string label = entry.displayName.empty() ? std::string("Unnamed Clip") : entry.displayName;
            const std::string sourceName = entry.sourceModelPath.empty()
                                               ? std::string("No source")
                                               : std::filesystem::path(entry.sourceModelPath).filename().string();
            const char *compatibility = CompatibilityLabel(anim, entry);
            if (m_LibraryFilter[0] != '\0' &&
                label.find(m_LibraryFilter) == std::string::npos &&
                sourceName.find(m_LibraryFilter) == std::string::npos)
                continue;

            ImGui::PushID(static_cast<int>(5000 + i));
            ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_Disabled);
            ImGui::SameLine();
            ImGui::TextColored(CompatibilityColor(compatibility), "%s", compatibility);
            ImGui::TextDisabled("%s", sourceName.c_str());
            ImGui::TextDisabled("Clip %d | %.2fs | %d/%d channels match",
                                entry.sourceClipIndex,
                                entry.durationSeconds,
                                CountMatchingChannelBones(anim, entry),
                                static_cast<int>(entry.channelBoneNames.size()));
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::EndChild();
    }

    void AnimationGraphPanel::StartLibraryImport(const std::vector<std::string> &files, uint32_t entityId)
    {
        if (m_ImportInProgress || files.empty())
            return;

        m_ImportInProgress = true;
        m_ImportAddedCount = 0;
        m_ImportTargetEntityId = entityId;

        const std::vector<std::string> filesCopy = files;
        m_ImportFuture = std::async(std::launch::async, [filesCopy]()
                                    { return BuildLibraryEntriesFromFiles(filesCopy); });
    }

    void AnimationGraphPanel::PumpLibraryImport(AnimationComponent *anim, uint32_t entityId)
    {
        if (!m_ImportInProgress || !m_ImportFuture.valid())
            return;

        if (m_ImportFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            return;

        std::vector<AnimationComponent::AnimationLibraryEntry> importedEntries = m_ImportFuture.get();
        m_ImportInProgress = false;

        if (!anim || entityId != m_ImportTargetEntityId)
            return;

        int addedCount = 0;
        for (auto &entry : importedEntries)
        {
            if (FindLibraryEntryIndex(anim, entry.sourceModelPath, entry.sourceClipIndex) != -1)
                continue;

            entry.displayName = MakeUniqueDisplayName(anim, NormalizeClipDisplayName(entry.displayName));
            anim->animationLibrary.push_back(std::move(entry));
            ++addedCount;
        }

        m_ImportAddedCount = addedCount;
    }

    void AnimationGraphPanel::DrawInspectorPane(AnimationComponent *anim)
    {
        ImGui::TextUnformatted("Inspector");
        ImGui::Separator();

        if (m_SelectedStateIndex >= 0 && m_SelectedStateIndex < static_cast<int>(anim->graphStates.size()))
        {
            auto &state = anim->graphStates[static_cast<size_t>(m_SelectedStateIndex)];
            ImGui::Text("State %d", m_SelectedStateIndex);

            char stateName[128] = {};
            std::strncpy(stateName, state.name.c_str(), sizeof(stateName) - 1);
            if (ImGui::InputText("Name", stateName, sizeof(stateName)))
                state.name = stateName;

            const auto *selectedLibraryEntry = GetLibraryEntry(anim, state.libraryClip);
            const std::string clipLabel = selectedLibraryEntry ? selectedLibraryEntry->displayName : std::string("No clip");
            if (ImGui::BeginCombo("Assigned Clip", clipLabel.c_str()))
            {
                for (size_t i = 0; i < anim->animationLibrary.size(); ++i)
                {
                    const auto &entry = anim->animationLibrary[i];
                    const bool selected = (state.libraryClip == static_cast<int>(i));
                    const std::string optionLabel = entry.displayName + "##libraryClip" + std::to_string(i);
                    if (ImGui::Selectable(optionLabel.c_str(), selected))
                    {
                        state.libraryClip = static_cast<int>(i);
                        state.modelPath = entry.sourceModelPath;
                        state.clipIndex = entry.sourceClipIndex;
                        state.durationSeconds = entry.durationSeconds;
                        state.rootTranslationDelta = entry.rootTranslationDelta;
                        state.rootRotationDeltaEuler = entry.rootRotationDeltaEuler;
                        state.rootScaleDelta = entry.rootScaleDelta;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Checkbox("Loop", &state.loop);
            ImGui::Text("Duration %.2fs", state.durationSeconds);

            ImGui::Separator();
            ImGui::TextUnformatted("Root Motion");
            ImGui::Checkbox("Enabled##rootMotion", &state.rootMotionEnabled);
            ImGui::BeginDisabled(!state.rootMotionEnabled);
            ImGui::Checkbox("Allow Vertical (Y)##rootMotion", &state.rootMotionAllowVertical);
            ImGui::SliderFloat("Distance Scale##rootMotion", &state.rootMotionScale, 0.0f, 5.0f, "%.2f");
            ImGui::SliderFloat("Max Speed (m/s)##rootMotion", &state.rootMotionMaxSpeed, 0.0f, 20.0f, "%.2f");
            ImGui::EndDisabled();

            ImGui::BeginDisabled(selectedLibraryEntry == nullptr);
            if (ImGui::Button("Reset Root Motion To Clip Defaults"))
            {
                state.durationSeconds = selectedLibraryEntry->durationSeconds;
                state.rootTranslationDelta = selectedLibraryEntry->rootTranslationDelta;
                state.rootRotationDeltaEuler = selectedLibraryEntry->rootRotationDeltaEuler;
                state.rootScaleDelta = selectedLibraryEntry->rootScaleDelta;
                state.rootMotionEnabled = true;
                state.rootMotionAllowVertical = false;
                state.rootMotionScale = 1.0f;
                state.rootMotionMaxSpeed = 4.0f;
            }
            ImGui::EndDisabled();

            if (selectedLibraryEntry)
            {
                const char *compatibility = CompatibilityLabel(anim, *selectedLibraryEntry);
                ImGui::TextColored(CompatibilityColor(compatibility), "Compatibility: %s", compatibility);
                ImGui::TextDisabled("Matching channels: %d/%d",
                                    CountMatchingChannelBones(anim, *selectedLibraryEntry),
                                    static_cast<int>(selectedLibraryEntry->channelBoneNames.size()));
                ImGui::TextDisabled("Source: %s", selectedLibraryEntry->sourceModelPath.c_str());
            }
            if (ImGui::Button("Set As Entry"))
                anim->activeState = m_SelectedStateIndex;
        }
        else if (m_SelectedTransitionIndex >= 0 && m_SelectedTransitionIndex < static_cast<int>(anim->graphTransitions.size()))
        {
            auto &transition = anim->graphTransitions[static_cast<size_t>(m_SelectedTransitionIndex)];
            ImGui::Text("Transition %d", m_SelectedTransitionIndex);
            ImGui::Text("From %d", transition.fromState);
            ImGui::Text("To %d", transition.toState);

            char transitionTrigger[128] = {};
            std::strncpy(transitionTrigger, transition.trigger.c_str(), sizeof(transitionTrigger) - 1);
            if (ImGui::InputText("Trigger", transitionTrigger, sizeof(transitionTrigger)))
                transition.trigger = transitionTrigger;

            if (ImGui::Button("Add Input Binding For Trigger"))
            {
                bool alreadyBound = false;
                for (const auto &binding : anim->inputBindings)
                {
                    if (binding.trigger == transition.trigger)
                    {
                        alreadyBound = true;
                        break;
                    }
                }

                if (!alreadyBound)
                {
                    AnimationComponent::InputBinding binding;
                    binding.trigger = transition.trigger;
                    binding.key = GLFW_KEY_UNKNOWN;
                    anim->inputBindings.push_back(binding);
                }
            }
        }
        else
        {
            const GraphValidationSummary summary = ValidateGraph(anim);
            ImGui::TextUnformatted("Graph");
            ImGui::Checkbox("Graph Enabled", &anim->graphEnabled);
            ImGui::Text("Entry State %d", anim->activeState);
            ImGui::Text("Library Clips %d", static_cast<int>(anim->animationLibrary.size()));
            ImGui::Text("States %d", static_cast<int>(anim->graphStates.size()));
            ImGui::Text("Transitions %d", static_cast<int>(anim->graphTransitions.size()));
            ImGui::Separator();
            ImGui::Text("Validation");
            ImGui::TextColored(summary.errors > 0 ? ImVec4(0.88f, 0.32f, 0.32f, 1.0f) : ImVec4(0.55f, 0.55f, 0.60f, 1.0f),
                               "Errors: %d", summary.errors);
            ImGui::TextColored(summary.warnings > 0 ? ImVec4(0.90f, 0.72f, 0.20f, 1.0f) : ImVec4(0.55f, 0.55f, 0.60f, 1.0f),
                               "Warnings: %d", summary.warnings);

            ImGui::Separator();
            ImGui::Text("Input Bindings");
            if (ImGui::Button("Add Input Binding"))
            {
                AnimationComponent::InputBinding binding;
                binding.trigger = "trigger";
                binding.key = GLFW_KEY_UNKNOWN;
                anim->inputBindings.push_back(binding);
            }

            for (size_t i = 0; i < anim->inputBindings.size(); ++i)
            {
                auto &binding = anim->inputBindings[i];
                ImGui::PushID(static_cast<int>(7000 + i));

                char bindingTrigger[128] = {};
                std::strncpy(bindingTrigger, binding.trigger.c_str(), sizeof(bindingTrigger) - 1);
                if (ImGui::InputText("Trigger", bindingTrigger, sizeof(bindingTrigger)))
                    binding.trigger = bindingTrigger;

                if (ImGui::BeginCombo("Key", KeyToLabel(binding.key)))
                {
                    for (const auto &opt : kKeyOptions)
                    {
                        const bool selected = (binding.key == opt.key);
                        if (ImGui::Selectable(opt.label, selected))
                            binding.key = opt.key;
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::Button("Remove Binding"))
                {
                    anim->inputBindings.erase(anim->inputBindings.begin() + static_cast<long long>(i));
                    ImGui::PopID();
                    break;
                }

                ImGui::Separator();
                ImGui::PopID();
            }

            ImGui::TextDisabled("Select a node or link to edit details.");
        }
    }

    void AnimationGraphPanel::DrawStatusBar(AnimationComponent *anim)
    {
        const GraphValidationSummary summary = ValidateGraph(anim);
        const char *activeStateName = "None";
        if (anim->activeState >= 0 && anim->activeState < static_cast<int>(anim->graphStates.size()))
            activeStateName = anim->graphStates[static_cast<size_t>(anim->activeState)].name.c_str();

        const char *selectionLabel = "None";
        if (m_SelectedStateIndex >= 0)
            selectionLabel = "State";
        else if (m_SelectedTransitionIndex >= 0)
            selectionLabel = "Transition";

        ImGui::Separator();
        ImGui::Text("Active State: %s", activeStateName);
        ImGui::SameLine();
        ImGui::TextDisabled("| Selected: %s", selectionLabel);
        ImGui::SameLine();
        ImGui::TextDisabled("| Validation: %d errors, %d warnings", summary.errors, summary.warnings);
    }

    void AnimationGraphPanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        if (!m_FilesScanned)
            RefreshAnimationFiles();

        if (!m_Scene)
        {
            ImGui::Text("No scene loaded");
            ImGui::End();
            return;
        }

        Entity *entity = nullptr;
        if (m_Hierarchy)
            entity = m_Scene->FindEntityByID(m_Hierarchy->GetSelectedEntityID());

        if (!entity)
        {
            ImGui::Text("Select a character entity in Hierarchy.");
            ImGui::End();
            return;
        }

        auto *anim = entity->GetComponent<AnimationComponent>();
        if (!anim)
            anim = &entity->AddComponent<AnimationComponent>();

        if (m_LastEntityId != entity->GetID())
        {
            m_LastEntityId = entity->GetID();
            m_SelectedStateIndex = -1;
            m_SelectedTransitionIndex = -1;
            m_LastLaidOutStateCount = 0;
            m_RequestRestorePositions = true;
        }

        PumpLibraryImport(anim, entity->GetID());

        for (auto &entry : anim->animationLibrary)
        {
            if (IsGenericMixamoName(entry.displayName) && !entry.sourceModelPath.empty())
            {
                entry.displayName = NormalizeClipDisplayName(std::filesystem::path(entry.sourceModelPath).stem().string());
            }
            else
            {
                entry.displayName = NormalizeClipDisplayName(entry.displayName);
            }
        }

        const char *entityName = "Entity";
        if (auto *name = entity->GetComponent<NameComponent>())
            entityName = name->Name.c_str();

        ImGui::Text("Target Character: %s", entityName);
        ImGui::SameLine();
        ImGui::Checkbox("Graph Enabled", &anim->graphEnabled);
        ImGui::SameLine();
        if (ImGui::Button("Add State"))
        {
            AnimationComponent::GraphState state = CreateBlankState(anim->graphStates.size());
            anim->graphStates.push_back(state);
            m_RequestAutoLayout = true;
            m_RequestRestorePositions = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Auto Layout"))
            m_RequestAutoLayout = true;
        ImGui::SameLine();
        if (ImGui::Button("Focus Active"))
        {
            if (anim->activeState >= 0 && anim->activeState < static_cast<int>(anim->graphStates.size()))
                m_RequestFocusNodeId = NodeIdFromIndex(static_cast<size_t>(anim->activeState));
            else
                m_RequestFocusNodeId = kEntryNodeId;
        }
        ImGui::SameLine();
        if (ImGui::Button("Fit View"))
            m_RequestFitView = true;
        ImGui::SameLine();
        ImGui::SetNextItemWidth(180.0f);
        ImGui::InputTextWithHint("##stateSearch", "Find state...", m_StateSearch, sizeof(m_StateSearch));

        ImGui::Separator();
        if (ImGui::BeginTable("AnimationGraphLayout", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Library", ImGuiTableColumnFlags_WidthStretch, 0.18f);
            ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthStretch, 0.64f);
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch, 0.18f);

            ImGui::TableNextColumn();
            ImGui::BeginChild("LibraryPane", ImVec2(0.0f, 0.0f), false);
            DrawLibraryPane(anim);
            ImGui::EndChild();

            ImGui::TableNextColumn();
            ImGui::BeginChild("GraphPane", ImVec2(0.0f, 0.0f), false);
            DrawGraphCanvas(anim);
            ImGui::EndChild();

            ImGui::TableNextColumn();
            ImGui::BeginChild("InspectorPane", ImVec2(0.0f, 0.0f), false);
            DrawInspectorPane(anim);
            ImGui::EndChild();

            ImGui::EndTable();
        }

        DrawStatusBar(anim);

        ImGui::End();
    }
}
