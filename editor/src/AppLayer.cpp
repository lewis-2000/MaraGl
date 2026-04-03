#include "AppLayer.h"
#include "AnimationComponent.h"
#include "Animator.h"
#include "MeshComponent.h"
#include "Model.h"
#include "NameComponent.h"
#include "Scene.h"
#include "SceneSerializer.h"
#include "Shader.h"
#include "TransformComponent.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <stb_image.h>

namespace MaraGl
{
    namespace
    {
        struct ClipCacheEntry
        {
            std::vector<Animation> clips;
            bool hasAnimations = false;
        };

        std::unordered_map<std::string, ClipCacheEntry> g_ExternalClipCache;

        int ClampClipIndex(const AnimationComponent *animComp, int clipIndex)
        {
            if (!animComp || animComp->animations.empty())
                return 0;
            return std::clamp(clipIndex, 0, static_cast<int>(animComp->animations.size()) - 1);
        }

        const std::vector<Animation> &LoadExternalClipsCached(const std::string &modelPath)
        {
            auto it = g_ExternalClipCache.find(modelPath);
            if (it != g_ExternalClipCache.end())
                return it->second.clips;

            ClipCacheEntry entry;
            try
            {
                Model sourceModel(modelPath);
                if (sourceModel.HasAnimations())
                {
                    entry.clips = sourceModel.LoadAnimations();
                    entry.hasAnimations = !entry.clips.empty();
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "[AppLayer] Failed to load external animation source '" << modelPath
                          << "': " << e.what() << std::endl;
            }

            auto inserted = g_ExternalClipCache.emplace(modelPath, std::move(entry));
            return inserted.first->second.clips;
        }

        int CountMatchingBones(const AnimationComponent *animComp,
                               const AnimationComponent::AnimationLibraryEntry &libraryEntry)
        {
            if (!animComp)
                return 0;

            int matches = 0;
            for (const auto &boneName : libraryEntry.channelBoneNames)
            {
                if (animComp->boneInfoMap.find(boneName) != animComp->boneInfoMap.end())
                    ++matches;
            }
            return matches;
        }

        int FindOrAppendMatchingClip(AnimationComponent *animComp,
                                     AnimationComponent::AnimationLibraryEntry &libraryEntry)
        {
            if (!animComp)
                return -1;

            if (libraryEntry.resolvedAnimationIndex >= 0 &&
                libraryEntry.resolvedAnimationIndex < static_cast<int>(animComp->animations.size()))
            {
                return libraryEntry.resolvedAnimationIndex;
            }

            if (libraryEntry.sourceModelPath.empty())
                return -1;

            if (!libraryEntry.channelBoneNames.empty() && CountMatchingBones(animComp, libraryEntry) == 0)
            {
                std::cerr << "[AppLayer] Rejecting incompatible clip '" << libraryEntry.displayName
                          << "' for target skeleton. No animation channels match target bones." << std::endl;
                return -1;
            }

            const std::vector<Animation> &externalClips = LoadExternalClipsCached(libraryEntry.sourceModelPath);
            if (externalClips.empty() || libraryEntry.sourceClipIndex < 0 || libraryEntry.sourceClipIndex >= static_cast<int>(externalClips.size()))
                return -1;

            const Animation &targetClip = externalClips[static_cast<size_t>(libraryEntry.sourceClipIndex)];
            for (size_t i = 0; i < animComp->animations.size(); ++i)
            {
                const Animation &existing = animComp->animations[i];
                if (existing.name == targetClip.name &&
                    std::abs(existing.duration - targetClip.duration) < 0.0001f &&
                    std::abs(existing.ticksPerSecond - targetClip.ticksPerSecond) < 0.0001f)
                {
                    libraryEntry.resolvedAnimationIndex = static_cast<int>(i);
                    return static_cast<int>(i);
                }
            }

            animComp->animations.push_back(targetClip);
            libraryEntry.resolvedAnimationIndex = static_cast<int>(animComp->animations.size() - 1);
            return libraryEntry.resolvedAnimationIndex;
        }

        void ResolveStateAnimation(AnimationComponent *animComp, int stateIndex)
        {
            if (!animComp || stateIndex < 0 || stateIndex >= static_cast<int>(animComp->graphStates.size()))
                return;

            auto &state = animComp->graphStates[static_cast<size_t>(stateIndex)];
            int resolvedIndex = -1;

            if (state.libraryClip >= 0 && state.libraryClip < static_cast<int>(animComp->animationLibrary.size()))
            {
                auto &libraryEntry = animComp->animationLibrary[static_cast<size_t>(state.libraryClip)];
                resolvedIndex = FindOrAppendMatchingClip(animComp, libraryEntry);
                if (state.modelPath.empty())
                    state.modelPath = libraryEntry.sourceModelPath;
                state.clipIndex = libraryEntry.sourceClipIndex;
                state.durationSeconds = libraryEntry.durationSeconds;
                state.rootTranslationDelta = libraryEntry.rootTranslationDelta;
                state.rootRotationDeltaEuler = libraryEntry.rootRotationDeltaEuler;
                state.rootScaleDelta = libraryEntry.rootScaleDelta;
            }
            else if (state.modelPath.empty())
            {
                resolvedIndex = ClampClipIndex(animComp, state.clipIndex);
            }
            else
            {
                AnimationComponent::AnimationLibraryEntry fallbackEntry;
                fallbackEntry.displayName = state.name;
                fallbackEntry.sourceModelPath = state.modelPath;
                fallbackEntry.sourceClipIndex = state.clipIndex;
                fallbackEntry.durationSeconds = state.durationSeconds;
                fallbackEntry.rootTranslationDelta = state.rootTranslationDelta;
                fallbackEntry.rootRotationDeltaEuler = state.rootRotationDeltaEuler;
                fallbackEntry.rootScaleDelta = state.rootScaleDelta;
                resolvedIndex = FindOrAppendMatchingClip(animComp, fallbackEntry);
            }

            if (resolvedIndex >= 0 && resolvedIndex < static_cast<int>(animComp->animations.size()))
            {
                animComp->currentAnimation = resolvedIndex;
                state.durationSeconds = (animComp->animations[static_cast<size_t>(resolvedIndex)].ticksPerSecond > 0.0f)
                                            ? (animComp->animations[static_cast<size_t>(resolvedIndex)].duration /
                                               animComp->animations[static_cast<size_t>(resolvedIndex)].ticksPerSecond)
                                            : 0.0f;
            }
        }

        void SetupEntityAnimation(Entity *entity, bool autoPlay)
        {
            if (!entity)
                return;

            auto *meshComp = entity->GetComponent<MeshComponent>();
            if (!meshComp || !meshComp->ModelPtr)
                return;

            AnimationComponent *animComp = entity->GetComponent<AnimationComponent>();
            if (!animComp)
                animComp = &entity->AddComponent<AnimationComponent>();

            animComp->boneInfoMap = meshComp->ModelPtr->GetBoneInfoMap();

            const int boneCount = meshComp->ModelPtr->GetBoneCount();
            animComp->boneTransforms.assign(static_cast<size_t>(boneCount), glm::mat4(1.0f));

            const bool graphMode = animComp->graphEnabled;
            const bool hasGraphState = graphMode && !animComp->graphStates.empty();

            if (graphMode)
            {
                // Graph mode owns runtime clip selection. Do not keep model-embedded clips.
                animComp->animations.clear();
                for (auto &libraryEntry : animComp->animationLibrary)
                    libraryEntry.resolvedAnimationIndex = -1;

                if (!hasGraphState)
                {
                    animComp->currentAnimation = 0;
                    animComp->currentTime = 0.0f;
                    animComp->playbackSpeed = 1.0f;
                    animComp->playing = false;
                    animComp->ClearBlendState();
                    animComp->ClearQueuedAnimation();
                    return;
                }

                animComp->activeState = std::clamp(animComp->activeState, 0, static_cast<int>(animComp->graphStates.size()) - 1);
                const auto &state = animComp->graphStates[static_cast<size_t>(animComp->activeState)];
                animComp->currentAnimation = state.clipIndex;
                animComp->looping = state.loop;
                ResolveStateAnimation(animComp, animComp->activeState);
            }
            else
            {
                animComp->animations = meshComp->ModelPtr->LoadAnimations();
                if (animComp->animations.empty())
                {
                    animComp->currentAnimation = 0;
                    animComp->currentTime = 0.0f;
                    animComp->playbackSpeed = 1.0f;
                    animComp->playing = false;
                    return;
                }

                animComp->currentAnimation = 0;
                animComp->looping = true;
            }

            if (animComp->animations.empty())
            {
                animComp->currentAnimation = 0;
                animComp->currentTime = 0.0f;
                animComp->playbackSpeed = 1.0f;
                animComp->playing = false;
                animComp->ClearBlendState();
                animComp->ClearQueuedAnimation();
                return;
            }

            animComp->currentAnimation = ClampClipIndex(animComp, animComp->currentAnimation);
            animComp->currentTime = 0.0f;
            animComp->playbackSpeed = 1.0f;
            animComp->playing = autoPlay;
            animComp->wasPlayingLastFrame = animComp->playing;
            animComp->ClearBlendState();
            animComp->ClearQueuedAnimation();
            animComp->graphTransitionActive = false;
            animComp->pendingGraphState = -1;

            const aiScene *scene = meshComp->ModelPtr->GetScene();
            if (scene && scene->mRootNode)
            {
                glm::mat4 identity(1.0f);
                glm::mat4 globalInverseTransform = Animator::GetGlobalInverseTransform(scene);
                Animator::CalculateBoneTransform(animComp, animComp->animations[static_cast<size_t>(animComp->currentAnimation)], scene->mRootNode, identity, globalInverseTransform);
            }
        }

        bool ApplyGraphTransition(AnimationComponent *animComp,
                                  const AnimationComponent::GraphTransition &transition,
                                  bool autoPlay)
        {
            if (!animComp)
                return false;

            if (transition.toState < 0 || transition.toState >= static_cast<int>(animComp->graphStates.size()))
                return false;

            if (!animComp->IsValidAnimationIndex(animComp->currentAnimation))
                return false;

            const int sourceAnimation = animComp->currentAnimation;
            const float sourceTime = animComp->currentTime;
            const bool sourceLooping = animComp->looping;

            auto &targetState = animComp->graphStates[static_cast<size_t>(transition.toState)];
            const int sourceState = animComp->activeState;

            ResolveStateAnimation(animComp, transition.toState);
            const int resolvedTargetAnimation = animComp->currentAnimation;

            animComp->currentAnimation = sourceAnimation;
            animComp->currentTime = sourceTime;
            animComp->looping = sourceLooping;
            animComp->playing = autoPlay;

            if (transition.blendDuration > 0.0001f && animComp->IsValidAnimationIndex(resolvedTargetAnimation))
            {
                animComp->PlayAnimation(resolvedTargetAnimation, targetState.loop, transition.blendDuration);
                animComp->graphTransitionActive = true;
                animComp->pendingGraphState = transition.toState;
            }
            else if (animComp->IsValidAnimationIndex(resolvedTargetAnimation))
            {
                animComp->currentAnimation = resolvedTargetAnimation;
                animComp->currentTime = 0.0f;
                animComp->looping = targetState.loop;
                animComp->activeState = transition.toState;
                animComp->ClearBlendState();
                animComp->graphTransitionActive = false;
                animComp->pendingGraphState = -1;
            }
            else
            {
                animComp->activeState = sourceState;
                animComp->currentAnimation = sourceAnimation;
                animComp->currentTime = sourceTime;
                animComp->looping = sourceLooping;
                return false;
            }

            return true;
        }

        bool ApplyGraphExitTransition(Entity *entity, bool autoPlay)
        {
            if (!entity)
                return false;

            auto *animComp = entity->GetComponent<AnimationComponent>();
            if (!animComp || !animComp->graphEnabled || animComp->graphStates.empty())
                return false;

            if (animComp->graphTransitionActive)
                return false;

            if (animComp->currentAnimation < 0 || animComp->currentAnimation >= static_cast<int>(animComp->animations.size()))
                return false;

            const Animation &currentAnim = animComp->animations[static_cast<size_t>(animComp->currentAnimation)];
            if (currentAnim.duration <= 0.0001f)
                return false;

            const float normalizedTime = std::clamp(animComp->currentTime / currentAnim.duration, 0.0f, 1.0f);
            for (const auto &transition : animComp->graphTransitions)
            {
                if (transition.fromState != animComp->activeState && transition.fromState != -1)
                    continue;
                if (!transition.hasExitTime || !transition.trigger.empty())
                    continue;

                const float exitTime = std::clamp(transition.exitTimeNormalized, 0.0f, 1.0f);
                if (normalizedTime + 0.0001f < exitTime)
                    continue;

                return ApplyGraphTransition(animComp, transition, autoPlay);
            }

            return false;
        }

        bool ApplyGraphTrigger(Entity *entity, const std::string &trigger, bool autoPlay)
        {
            if (!entity)
                return false;

            auto *animComp = entity->GetComponent<AnimationComponent>();
            if (!animComp || !animComp->graphEnabled || animComp->graphStates.empty())
                return false;

            if (animComp->graphTransitionActive)
                return false;

            float normalizedTime = 0.0f;
            if (animComp->currentAnimation >= 0 && animComp->currentAnimation < static_cast<int>(animComp->animations.size()))
            {
                const Animation &currentAnim = animComp->animations[static_cast<size_t>(animComp->currentAnimation)];
                if (currentAnim.duration > 0.0001f)
                    normalizedTime = std::clamp(animComp->currentTime / currentAnim.duration, 0.0f, 1.0f);
            }

            for (const auto &transition : animComp->graphTransitions)
            {
                if (transition.fromState != animComp->activeState && transition.fromState != -1)
                    continue;
                if (transition.trigger != trigger)
                    continue;
                if (transition.hasExitTime)
                {
                    const float exitTime = std::clamp(transition.exitTimeNormalized, 0.0f, 1.0f);
                    if (normalizedTime + 0.0001f < exitTime)
                        continue;
                }

                return ApplyGraphTransition(animComp, transition, autoPlay);
            }

            return false;
        }
    }

    AppLayer::AppLayer(int width, int height, const char *title)
        : m_Window(width, height, title),
          m_Renderer(),
          m_ImGuiLayer(m_Window),
          m_Framebuffer(width, height)
    {
        Input::Init(m_Window.getWindow());

        m_ImGuiLayer.SetScene(&m_Scene);
        m_ImGuiLayer.SetAssetLoader(&m_AssetLoader);
        m_PreviousKeyState.fill(false);

        // Editor should show light gizmos for authoring.
        m_Scene.SetLightGizmoVisible(true);
    }

    AppLayer::~AppLayer()
    {
        if (m_SplashLogoTextureID != 0)
        {
            glDeleteTextures(1, &m_SplashLogoTextureID);
            m_SplashLogoTextureID = 0;
        }

        Mesh::ClearTextureCache();
    }

    void AppLayer::EnsureSplashLogoLoaded()
    {
        if (m_SplashLogoTextureID != 0)
            return;

        constexpr const char *logoPath = "resources/Images/Logo.png";

        stbi_set_flip_vertically_on_load(true);
        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char *pixels = stbi_load(logoPath, &width, &height, &channels, 4);
        if (!pixels)
        {
            std::cerr << "[AppLayer] Failed to load splash logo: " << logoPath << std::endl;
            return;
        }

        // Convert bright neutral background pixels to transparent so logos exported on light canvas blend on dark UI.
        const int pixelCount = width * height;
        for (int i = 0; i < pixelCount; ++i)
        {
            unsigned char *p = &pixels[i * 4];
            const int r = static_cast<int>(p[0]);
            const int g = static_cast<int>(p[1]);
            const int b = static_cast<int>(p[2]);
            const int maxCh = std::max(r, std::max(g, b));
            const int minCh = std::min(r, std::min(g, b));
            const int chroma = maxCh - minCh;

            if (minCh > 212 && chroma < 16)
            {
                p[3] = 0;
            }
            else if (minCh > 188 && chroma < 20)
            {
                const float t = static_cast<float>(minCh - 188) / 24.0f;
                const float fade = std::clamp(1.0f - t, 0.0f, 1.0f);
                p[3] = static_cast<unsigned char>(static_cast<float>(p[3]) * fade);
            }
        }

        glGenTextures(1, &m_SplashLogoTextureID);
        glBindTexture(GL_TEXTURE_2D, m_SplashLogoTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(pixels);

        m_SplashLogoWidth = width;
        m_SplashLogoHeight = height;
    }

    float AppLayer::ComputeStartupProgress()
    {
        const int pendingLoads = std::max(0, m_PendingModelLoads.load());
        const int totalLoads = std::max(0, m_StartupModelLoadsTotal);

        float progress = 0.2f; // Scene JSON parse + setup is done before async model load tracking.
        if (m_SceneLoaded)
            progress = 0.35f;

        if (totalLoads > 0)
        {
            const int completed = std::max(0, totalLoads - pendingLoads);
            const float modelProgress = static_cast<float>(completed) / static_cast<float>(totalLoads);
            progress = std::max(progress, 0.35f + (modelProgress * 0.60f));
        }

        const auto loaderItems = m_AssetLoader.GetLoadingProgress();
        if (!loaderItems.empty())
        {
            float avg = 0.0f;
            for (const auto &item : loaderItems)
                avg += std::clamp(item.progress, 0.0f, 1.0f);
            avg /= static_cast<float>(loaderItems.size());
            progress = std::max(progress, 0.35f + (avg * 0.60f));
        }

        const bool startupReady = m_SceneLoaded && pendingLoads == 0 && !m_AssetLoader.IsLoading();
        if (startupReady)
            progress = 1.0f;

        return std::clamp(progress, 0.0f, 1.0f);
    }

    void AppLayer::RenderStartupSplash(float deltaTime)
    {
        if (!m_ShowStartupSplash)
            return;

        EnsureSplashLogoLoaded();

        m_StartupElapsedSeconds += std::max(0.0f, deltaTime);
        m_StartupProgress = ComputeStartupProgress();

        constexpr float minVisibleSeconds = 0.7f;
        constexpr float fadeOutSeconds = 0.25f;

        const bool startupReady = m_StartupProgress >= 0.999f;
        if (!m_SplashFadeOutStarted && startupReady && m_StartupElapsedSeconds >= minVisibleSeconds)
            m_SplashFadeOutStarted = true;

        if (m_SplashFadeOutStarted)
            m_SplashAlpha = std::max(0.0f, m_SplashAlpha - (deltaTime / fadeOutSeconds));

        if (m_SplashAlpha <= 0.001f)
        {
            m_ShowStartupSplash = false;
            return;
        }

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowBgAlpha(1.0f * m_SplashAlpha);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_NoDocking |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoInputs;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.10f, 1.0f * m_SplashAlpha));
        if (!ImGui::Begin("StartupSplash", nullptr, flags))
        {
            ImGui::End();
            ImGui::PopStyleColor();
            return;
        }

        const ImVec2 contentSize = ImGui::GetContentRegionAvail();
        const float logoMaxWidth = std::min(contentSize.x * 0.5f, 520.0f);
        const float logoAspect = (m_SplashLogoHeight > 0) ? (static_cast<float>(m_SplashLogoWidth) / static_cast<float>(m_SplashLogoHeight)) : 2.5f;
        const float logoWidth = std::max(220.0f, logoMaxWidth);
        const float logoHeight = logoWidth / std::max(0.1f, logoAspect);
        const ImVec2 logoSize(logoWidth, logoHeight);

        const float progressWidth = std::min(contentSize.x * 0.45f, 420.0f);
        const float blockHeight = logoSize.y + 64.0f;
        const float startY = std::max(24.0f, (contentSize.y - blockHeight) * 0.5f);
        const float logoX = (contentSize.x - logoSize.x) * 0.5f;

        ImGui::SetCursorPos(ImVec2(logoX, startY));
        if (m_SplashLogoTextureID != 0)
        {
            ImGui::Image((ImTextureID)(uintptr_t)m_SplashLogoTextureID,
                         logoSize,
                         ImVec2(0.0f, 1.0f),
                         ImVec2(1.0f, 0.0f));
        }
        else
        {
            ImGui::Dummy(logoSize);
            ImVec2 pMin = ImGui::GetItemRectMin();
            ImVec2 pMax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(pMin, pMax, IM_COL32(35, 38, 44, static_cast<int>(170.0f * m_SplashAlpha)), 8.0f);
            ImGui::GetWindowDrawList()->AddText(ImVec2(pMin.x + 18.0f, pMin.y + logoSize.y * 0.5f - 10.0f),
                                                IM_COL32(220, 225, 235, static_cast<int>(255.0f * m_SplashAlpha)),
                                                "MaraGl");
        }

        ImGui::SetCursorPosX((contentSize.x - progressWidth) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.28f, 0.63f, 0.96f, 0.95f * m_SplashAlpha));
        ImGui::ProgressBar(m_StartupProgress, ImVec2(progressWidth, 0.0f));
        ImGui::PopStyleColor();

        const char *brandText = "Mara";
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.92f, 0.94f, 0.95f * m_SplashAlpha));
        ImGui::SetCursorPosX((contentSize.x - ImGui::CalcTextSize(brandText).x) * 0.5f);
        ImGui::TextUnformatted(brandText);
        ImGui::PopStyleColor();

        std::string statusText = "Preparing editor...";
        if (m_StartupProgress >= 0.999f)
            statusText = "Ready";
        else if (m_PendingModelLoads.load() > 0)
            statusText = "Loading scene assets...";
        else if (m_AssetLoader.IsLoading())
            statusText = "Loading startup scene...";

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.84f, 0.90f, 0.95f * m_SplashAlpha));
        ImGui::SetCursorPosX((contentSize.x - ImGui::CalcTextSize(statusText.c_str()).x) * 0.5f);
        ImGui::TextUnformatted(statusText.c_str());
        ImGui::PopStyleColor();

        ImGui::End();
        ImGui::PopStyleColor();
    }

    void AppLayer::processEvents()
    {
        m_Window.pollEvents();
        Input::Update();
    }

    void AppLayer::render()
    {
        m_Timer.Update();
        float deltaTime = m_Timer.GetDeltaTime();

        if (!m_StartupSceneRequested)
        {
            m_StartupSceneRequested = true;
            LoadScene("scenes/default_animation_scene.json");
        }

        m_AssetLoader.ProcessCompletedLoads();

        ScenePanel *scenePanel = m_ImGuiLayer.GetScenePanel();
        bool sceneFocused = scenePanel ? scenePanel->IsFocused() : false;
        HandleGraphInput(sceneFocused);

        // Apply root motion in game mode
        ApplyRootMotionInGameMode(deltaTime);

        m_Scene.Update(deltaTime);
        ProcessGraphRuntimeEvents();
        m_ImGuiLayer.Update(deltaTime);

        auto &camera = m_Renderer.GetCamera();
        const auto &savedCamera = m_Scene.GetCameraSettings();
        camera.SetEditorPosition(savedCamera.Position);
        camera.SetYawPitch(savedCamera.Yaw, savedCamera.Pitch);
        camera.SetMoveSpeed(savedCamera.MoveSpeed);
        camera.SetMouseSensitivity(savedCamera.MouseSensitivity);
        camera.SetFOV(savedCamera.FOV);
        camera.SetClipPlanes(savedCamera.NearClip, savedCamera.FarClip);
        camera.Update(deltaTime, sceneFocused);

        // Update camera position behind model in game mode
        UpdateCameraBehindModel();

        Scene::CameraSettings liveCamera;
        liveCamera.Position = camera.GetEditorPosition();
        liveCamera.Yaw = camera.GetYaw();
        liveCamera.Pitch = camera.GetPitch();
        liveCamera.MoveSpeed = camera.GetMoveSpeed();
        liveCamera.MouseSensitivity = camera.GetMouseSensitivity();
        liveCamera.FOV = camera.GetFOV();
        liveCamera.NearClip = camera.GetNearClip();
        liveCamera.FarClip = camera.GetFarClip();
        m_Scene.SetCameraSettings(liveCamera);

        static ::Shader shader("resources/shaders/basic.vert", "resources/shaders/basic.frag");

        ModelLoaderPanel *loaderPanel = m_ImGuiLayer.GetModelLoaderPanel();
        if (loaderPanel && loaderPanel->ShouldLoadModel())
        {
            std::string modelPath = loaderPanel->GetSelectedModelPath();
            std::filesystem::path path(modelPath);
            std::string entityName = path.stem().string();
            if (entityName.empty())
                entityName = "ModelEntity";

            m_AssetLoader.LoadModelAsync(modelPath, entityName,
                                         [this, modelPath, entityName](std::shared_ptr<Model> model, bool success, const std::string &errorMsg)
                                         {
                                             if (success && model)
                                             {
                                                 Entity &entity = m_Scene.CreateEntity(entityName);
                                                 auto &nameComp = entity.AddComponent<NameComponent>();
                                                 nameComp.Name = entityName;
                                                 entity.AddComponent<TransformComponent>();
                                                 auto &meshComp = entity.AddComponent<MeshComponent>();
                                                 meshComp.ModelPtr = model;
                                                 meshComp.ModelPath = modelPath;
                                                 SetupEntityAnimation(&entity, false);

                                                 if (auto *hierarchyPanel = m_ImGuiLayer.GetHierarchyPanel())
                                                     hierarchyPanel->SetSelectedEntityID(entity.GetID());
                                             }
                                             else
                                             {
                                                 std::cerr << "[AppLayer] Failed to load model: " << errorMsg << std::endl;
                                             }
                                         });
        }

        m_Framebuffer.bind();
        glViewport(0, 0, m_Framebuffer.getWidth(), m_Framebuffer.getHeight());
        glEnable(GL_DEPTH_TEST);
        m_Renderer.clear(0.1f, 0.2f, 0.3f, 1.0f);

        m_Scene.Render(m_Renderer, shader);
        m_Scene.RenderGrid(m_Renderer);

        m_Framebuffer.unbind();

        int ww = 0, hh = 0;
        glfwGetFramebufferSize(m_Window.getWindow(), &ww, &hh);
        glViewport(0, 0, ww, hh);

        m_ImGuiLayer.begin();
        if (m_ShowStartupSplash)
        {
            RenderStartupSplash(deltaTime);
        }
        else
        {
            m_ImGuiLayer.renderDockspace(&m_Framebuffer, &m_Renderer);
        }
        m_ImGuiLayer.end();

        if (!m_WindowShown)
        {
            m_Window.show();
            m_WindowShown = true;
        }

        glfwSwapBuffers(m_Window.getWindow());
    }

    bool AppLayer::WasKeyPressedOnce(int key)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            return false;

        bool isPressed = Input::IsKeyPressed(key);
        bool wasPressed = m_PreviousKeyState[static_cast<size_t>(key)];
        m_PreviousKeyState[static_cast<size_t>(key)] = isPressed;
        return isPressed && !wasPressed;
    }

    void AppLayer::QueueGraphModelLoad(uint32_t entityID, const std::string &modelPath, bool autoPlayAfterLoad)
    {
        m_PendingModelLoads++;
        m_AssetLoader.LoadModelAsync(modelPath, std::to_string(entityID),
                                     [this, entityID, modelPath, autoPlayAfterLoad](std::shared_ptr<Model> model, bool success, const std::string &errorMsg)
                                     {
                                         Entity *target = m_Scene.FindEntityByID(entityID);
                                         if (target)
                                         {
                                             auto *mesh = target->GetComponent<MeshComponent>();
                                             if (mesh && success && model)
                                             {
                                                 mesh->ModelPath = modelPath;
                                                 mesh->ModelPtr = model;
                                                 SetupEntityAnimation(target, autoPlayAfterLoad);
                                             }
                                             else if (!success)
                                             {
                                                 m_FailedModelLoads++;
                                                 std::cerr << "[AppLayer] Failed graph model load '" << modelPath << "': " << errorMsg << std::endl;
                                             }
                                         }
                                         m_PendingModelLoads--;
                                     });
    }

    void AppLayer::HandleGraphInput(bool sceneFocused)
    {
        if (WasKeyPressedOnce(GLFW_KEY_G))
        {
            m_GameMode = !m_GameMode;
            std::cout << "[AppLayer] Game mode " << (m_GameMode ? "enabled" : "disabled") << std::endl;
        }

        if (!m_GameMode || !sceneFocused)
            return;

        Entity *graphEntity = m_Scene.FindEntityByID(m_GraphEntityID);
        if (!graphEntity)
        {
            for (const auto &entity : m_Scene.GetEntities())
            {
                auto *animComp = entity->GetComponent<AnimationComponent>();
                if (animComp && animComp->graphEnabled && !animComp->graphStates.empty())
                {
                    m_GraphEntityID = entity->GetID();
                    graphEntity = entity.get();
                    break;
                }
            }
        }

        if (!graphEntity)
            return;

        auto *animComp = graphEntity->GetComponent<AnimationComponent>();
        if (!animComp || !animComp->graphEnabled)
            return;

        for (const auto &binding : animComp->inputBindings)
        {
            if (binding.key < 0 || !WasKeyPressedOnce(binding.key))
                continue;

            ApplyGraphTrigger(graphEntity, binding.trigger, true);
            return;
        }
    }

    void AppLayer::UpdateCameraBehindModel()
    {
        if (!m_GameMode)
            return;

        Entity *graphEntity = m_Scene.FindEntityByID(m_GraphEntityID);
        if (!graphEntity)
            return;

        auto *transform = graphEntity->GetComponent<TransformComponent>();
        if (!transform)
            return;

        auto &camera = m_Renderer.GetCamera();

        // Position camera behind the model
        const float cameraDistance = 5.0f;
        const float cameraHeight = 2.5f;

        // Get current camera yaw to determine direction
        float yaw = camera.GetYaw();
        float pitch = -0.3f; // Slight downward angle

        // Position camera behind model (opposite of current yaw direction)
        glm::vec3 cameraPos = transform->Position;
        cameraPos.x -= std::cos(yaw) * cameraDistance;
        cameraPos.z -= std::sin(yaw) * cameraDistance;
        cameraPos.y += cameraHeight;

        Scene::CameraSettings camSettings = m_Scene.GetCameraSettings();
        camSettings.Position = cameraPos;
        camSettings.Yaw = yaw;
        camSettings.Pitch = pitch;
        m_Scene.SetCameraSettings(camSettings);
    }

    void AppLayer::ApplyRootMotionInGameMode(float deltaTime)
    {
        if (!m_GameMode)
            return;

        Entity *graphEntity = m_Scene.FindEntityByID(m_GraphEntityID);
        if (!graphEntity)
            return;

        auto *transform = graphEntity->GetComponent<TransformComponent>();
        auto *animComp = graphEntity->GetComponent<AnimationComponent>();

        if (!transform || !animComp || !animComp->playing ||
            !animComp->graphEnabled || animComp->graphStates.empty())
            return;

        const int stateIndex = glm::clamp(animComp->activeState, 0,
                                          static_cast<int>(animComp->graphStates.size()) - 1);
        const auto &state = animComp->graphStates[static_cast<size_t>(stateIndex)];

        if (!state.rootMotionEnabled)
            return;

        if (state.durationSeconds <= 0.0001f)
            return;

        glm::vec3 cycleDelta = state.rootTranslationDelta;
        if (!state.rootMotionAllowVertical)
            cycleDelta.y = 0.0f; // Keep gameplay root motion planar unless explicitly enabled.

        const float cycleDistance = glm::length(cycleDelta);
        if (cycleDistance <= 0.0001f)
            return;

        // Mixamo sources often encode translation in larger authoring units (commonly centimeters).
        // Normalize large cycle deltas so world movement stays reasonable.
        float unitScale = 1.0f;
        if (cycleDistance > 15.0f)
            unitScale = 0.01f;

        const float rootMotionScale = std::max(0.0f, state.rootMotionScale);
        glm::vec3 velocity = ((cycleDelta * unitScale) / state.durationSeconds) * rootMotionScale;

        const float maxRootMotionSpeed = std::max(0.0f, state.rootMotionMaxSpeed);
        const float speed = glm::length(velocity);
        if (maxRootMotionSpeed > 0.0f && speed > maxRootMotionSpeed)
            velocity *= (maxRootMotionSpeed / speed);

        transform->Position += velocity * (deltaTime * animComp->playbackSpeed);
    }

    void AppLayer::ProcessGraphRuntimeEvents()
    {
        for (const auto &entity : m_Scene.GetEntities())
        {
            auto *animComp = entity->GetComponent<AnimationComponent>();
            if (!animComp || !animComp->graphEnabled || animComp->graphStates.empty())
                continue;

            ApplyGraphExitTransition(entity.get(), true);

            const bool completedThisFrame = (!animComp->playing && animComp->wasPlayingLastFrame && !animComp->looping);
            if (completedThisFrame &&
                animComp->activeState >= 0 &&
                animComp->activeState < static_cast<int>(animComp->graphStates.size()))
            {
                const auto &state = animComp->graphStates[static_cast<size_t>(animComp->activeState)];
                const std::string completeTrigger = "OnComplete:" + state.name;
                ApplyGraphTrigger(entity.get(), completeTrigger, true);
            }

            animComp->wasPlayingLastFrame = animComp->playing;
        }
    }

    void AppLayer::LoadScene(const std::string &scenePath)
    {
        std::cout << "[AppLayer] Queueing scene load: " << scenePath << std::endl;
        m_PendingModelLoads = 0;
        m_StartupModelLoadsTotal = 0;
        m_FailedModelLoads = 0;
        m_GraphEntityID = 0;
        m_SceneLoaded = false;
        m_Scene.ClearScene();
        m_Scene.SetLightGizmoVisible(true);

        m_AssetLoader.LoadSceneAsync(&m_Scene, scenePath,
                                     [this, scenePath](bool success, const std::string &errorMsg)
                                     {
                                         if (!success)
                                         {
                                             std::cerr << "[AppLayer] Failed to load scene '" << scenePath << "': " << errorMsg << std::endl;
                                             // Avoid getting stuck behind splash on startup errors.
                                             m_SceneLoaded = true;
                                             return;
                                         }

                                         for (const auto &entity : m_Scene.GetEntities())
                                         {
                                             auto *animComp = entity->GetComponent<AnimationComponent>();
                                             if (!m_GraphEntityID && animComp && animComp->graphEnabled && !animComp->graphStates.empty())
                                                 m_GraphEntityID = entity->GetID();

                                             auto *meshComp = entity->GetComponent<MeshComponent>();
                                             if (!meshComp || meshComp->ModelPath.empty())
                                                 continue;

                                             m_StartupModelLoadsTotal++;

                                             if (meshComp->ModelPtr)
                                             {
                                                 SetupEntityAnimation(entity.get(), false);
                                             }
                                             else
                                             {
                                                 const uint32_t entityID = entity->GetID();
                                                 const std::string modelPath = meshComp->ModelPath;
                                                 m_PendingModelLoads++;
                                                 m_AssetLoader.LoadModelAsync(modelPath, std::to_string(entityID),
                                                                              [this, entityID, modelPath](std::shared_ptr<Model> model, bool loaded, const std::string &loadError)
                                                                              {
                                                                                  Entity *target = m_Scene.FindEntityByID(entityID);
                                                                                  if (target)
                                                                                  {
                                                                                      auto *mesh = target->GetComponent<MeshComponent>();
                                                                                      if (mesh && loaded && model)
                                                                                      {
                                                                                          mesh->ModelPath = modelPath;
                                                                                          mesh->ModelPtr = model;
                                                                                          SetupEntityAnimation(target, false);
                                                                                      }
                                                                                      else if (!loaded)
                                                                                      {
                                                                                          m_FailedModelLoads++;
                                                                                          std::cerr << "[AppLayer] Failed async model load '" << modelPath << "': " << loadError << std::endl;
                                                                                      }
                                                                                  }
                                                                                  m_PendingModelLoads--;
                                                                              });
                                             }
                                         }

                                         m_SceneLoaded = true;
                                     });
    }

    void AppLayer::run()
    {
        while (!glfwWindowShouldClose(m_Window.getWindow()))
        {
            processEvents();
            render();
        }
    }

    void AppLayer::LoadSkybox(const std::string &path)
    {
        m_Scene.LoadSkybox(path);
    }
}
