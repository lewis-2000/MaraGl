#include "SandboxApp.h"
#include "ImGuiFontSetup.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "AnimationComponent.h"
#include "Animator.h"
#include "Model.h"
#include "SceneSerializer.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <sstream>
#include <iostream>
#include <cmath>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace MaraGl
{
    namespace
    {
        int ClampClipIndex(const AnimationComponent *animComp, int clipIndex)
        {
            if (!animComp || animComp->animations.empty())
                return 0;
            return glm::clamp(clipIndex, 0, static_cast<int>(animComp->animations.size()) - 1);
        }

        const char *KeyToLabel(int key)
        {
            switch (key)
            {
            case GLFW_KEY_UNKNOWN:
                return "None";
            case GLFW_KEY_SPACE:
                return "Space";
            case GLFW_KEY_LEFT_SHIFT:
                return "Left Shift";
            case GLFW_KEY_RIGHT_SHIFT:
                return "Right Shift";
            case GLFW_KEY_LEFT_CONTROL:
                return "Left Ctrl";
            case GLFW_KEY_RIGHT_CONTROL:
                return "Right Ctrl";
            case GLFW_KEY_ENTER:
                return "Enter";
            case GLFW_KEY_TAB:
                return "Tab";
            default:
                break;
            }

            static thread_local char label[8] = {};
            if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
            {
                label[0] = static_cast<char>('A' + (key - GLFW_KEY_A));
                label[1] = '\0';
                return label;
            }

            if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
            {
                label[0] = static_cast<char>('0' + (key - GLFW_KEY_0));
                label[1] = '\0';
                return label;
            }

            std::snprintf(label, sizeof(label), "%d", key);
            return label;
        }

        // ---- Library-resolution helpers (mirrors AppLayer) ----

        struct ClipCacheEntry
        {
            std::vector<Animation> clips;
        };
        std::unordered_map<std::string, ClipCacheEntry> g_ExternalClipCache;

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
                    entry.clips = sourceModel.LoadAnimations();
            }
            catch (const std::exception &e)
            {
                std::cerr << "[SandboxApp] Failed to load external clip source '" << modelPath << "': " << e.what() << std::endl;
            }
            auto inserted = g_ExternalClipCache.emplace(modelPath, std::move(entry));
            return inserted.first->second.clips;
        }

        int CountMatchingBones(const AnimationComponent *animComp,
                               const AnimationComponent::AnimationLibraryEntry &entry)
        {
            if (!animComp)
                return 0;
            int matches = 0;
            for (const auto &boneName : entry.channelBoneNames)
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
                return libraryEntry.resolvedAnimationIndex;

            if (libraryEntry.sourceModelPath.empty())
                return -1;

            if (!libraryEntry.channelBoneNames.empty() && CountMatchingBones(animComp, libraryEntry) == 0)
            {
                std::cerr << "[SandboxApp] Rejecting incompatible clip '" << libraryEntry.displayName << "'." << std::endl;
                return -1;
            }

            const std::vector<Animation> &externalClips = LoadExternalClipsCached(libraryEntry.sourceModelPath);
            if (externalClips.empty() ||
                libraryEntry.sourceClipIndex < 0 ||
                libraryEntry.sourceClipIndex >= static_cast<int>(externalClips.size()))
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
                const auto &clip = animComp->animations[static_cast<size_t>(resolvedIndex)];
                state.durationSeconds = (clip.ticksPerSecond > 0.0f)
                                            ? (clip.duration / clip.ticksPerSecond)
                                            : 0.0f;

                if (animComp->activeState == stateIndex)
                    animComp->PlayAnimation(resolvedIndex, state.loop, 0.0f, true);
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
                animComp->animations.clear();
                for (auto &libraryEntry : animComp->animationLibrary)
                    libraryEntry.resolvedAnimationIndex = -1;

                if (!hasGraphState)
                {
                    animComp->currentAnimation = 0;
                    animComp->currentTime = 0.0f;
                    animComp->playbackSpeed = 1.0f;
                    animComp->playing = false;
                    animComp->wasPlayingLastFrame = false;
                    return;
                }

                animComp->activeState = glm::clamp(animComp->activeState, 0, static_cast<int>(animComp->graphStates.size()) - 1);
                ResolveStateAnimation(animComp, animComp->activeState);
                animComp->looping = animComp->graphStates[static_cast<size_t>(animComp->activeState)].loop;
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
                    animComp->wasPlayingLastFrame = false;
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
                animComp->wasPlayingLastFrame = false;
                return;
            }

            animComp->currentAnimation = ClampClipIndex(animComp, animComp->currentAnimation);
            animComp->currentTime = 0.0f;
            animComp->playbackSpeed = 1.0f;
            animComp->playing = autoPlay;
            animComp->wasPlayingLastFrame = animComp->playing;

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

            animComp->activeState = transition.toState;
            ResolveStateAnimation(animComp, animComp->activeState);
            animComp->looping = animComp->graphStates[static_cast<size_t>(animComp->activeState)].loop;
            animComp->currentTime = 0.0f;
            animComp->playing = autoPlay;
            (void)transition.blendDuration;
            return true;
        }

        bool ApplyGraphExitTransition(Entity *entity, bool autoPlay)
        {
            if (!entity)
                return false;

            auto *animComp = entity->GetComponent<AnimationComponent>();
            if (!animComp || !animComp->graphEnabled || animComp->graphStates.empty())
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

    SandboxApp::SandboxApp(int width, int height, const char *title)
        : m_Window(width, height, title),
          m_Renderer(),
          m_Scene(),
          m_BaseTitle(title ? title : "MaraGl Sandbox")
    {
        Input::Init(m_Window.getWindow());

        // Set default camera position
        Scene::CameraSettings camera;
        camera.Position = glm::vec3(6.0f, 4.0f, 6.0f);
        camera.Yaw = -2.35619449f;
        camera.Pitch = -0.45f;
        m_Scene.SetCameraSettings(camera);
        m_PreviousKeyState.fill(false);
        RefreshAvailableScenes();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        ConfigureDefaultImGuiFonts(io, 20.0f, 20.0f);
        ImGui_ImplGlfw_InitForOpenGL(m_Window.getWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        std::cout << "[SandboxApp] Initialized" << std::endl;
    }

    SandboxApp::~SandboxApp()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void SandboxApp::processEvents()
    {
        m_Window.pollEvents();
        Input::Update();
    }

    void SandboxApp::update(float deltaTime)
    {
        HandleAnimationInput();

        ApplyRootMotion(deltaTime);

        // Update scene
        m_Scene.Update(deltaTime);
        ProcessGraphRuntimeEvents();

        // Update camera
        auto &camera = m_Renderer.GetCamera();
        const auto &savedCamera = m_Scene.GetCameraSettings();
        camera.SetEditorPosition(savedCamera.Position);
        camera.SetYawPitch(savedCamera.Yaw, savedCamera.Pitch);
        camera.SetMoveSpeed(savedCamera.MoveSpeed);
        camera.SetMouseSensitivity(savedCamera.MouseSensitivity);
        camera.SetFOV(savedCamera.FOV);
        camera.SetClipPlanes(savedCamera.NearClip, savedCamera.FarClip);
        camera.Update(deltaTime, !m_ThirdPersonMode);

        if (m_ThirdPersonMode)
            ApplyThirdPersonCamera(deltaTime);

        // Save updated camera
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
    }

    void SandboxApp::ProcessGraphRuntimeEvents()
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

    bool SandboxApp::WasKeyPressedOnce(int key)
    {
        if (key < 0 || key > GLFW_KEY_LAST)
            return false;

        bool isPressed = Input::IsKeyPressed(key);
        bool wasPressed = m_PreviousKeyState[key];
        m_PreviousKeyState[key] = isPressed;
        return isPressed && !wasPressed;
    }

    void SandboxApp::HandleAnimationInput()
    {
        if (WasKeyPressedOnce(GLFW_KEY_F5))
        {
            if (!m_CurrentScenePath.empty())
                LoadScene(m_CurrentScenePath);
            return;
        }

        if (WasKeyPressedOnce(GLFW_KEY_F1))
            m_ShowHud = !m_ShowHud;

        if (WasKeyPressedOnce(GLFW_KEY_RIGHT_BRACKET))
        {
            LoadNextScene();
            return;
        }

        if (WasKeyPressedOnce(GLFW_KEY_LEFT_BRACKET))
        {
            LoadPreviousScene();
            return;
        }

        if (WasKeyPressedOnce(GLFW_KEY_T))
            m_ThirdPersonMode = !m_ThirdPersonMode;

        if (WasKeyPressedOnce(GLFW_KEY_M))
            m_UseRootMotion = !m_UseRootMotion;

        Entity *player = m_Scene.FindEntityByID(m_PlayerEntityID);
        if (!player)
            return;

        auto *animComp = player->GetComponent<AnimationComponent>();
        if (!animComp)
            return;

        if (WasKeyPressedOnce(GLFW_KEY_P))
            animComp->playing = !animComp->playing;

        if (WasKeyPressedOnce(GLFW_KEY_R))
        {
            animComp->currentTime = 0.0f;
            animComp->ClearBlendState();
            animComp->ClearQueuedAnimation();
            animComp->graphTransitionActive = false;
            animComp->pendingGraphState = -1;
        }

        for (const auto &binding : animComp->inputBindings)
        {
            if (binding.key < 0 || !WasKeyPressedOnce(binding.key))
                continue;

            if (ApplyGraphTrigger(player, binding.trigger, true))
                return;
        }
    }

    void SandboxApp::ApplyRootMotion(float deltaTime)
    {
        if (!m_UseRootMotion)
            return;

        Entity *player = m_Scene.FindEntityByID(m_PlayerEntityID);
        if (!player)
            return;

        auto *transform = player->GetComponent<TransformComponent>();
        auto *animComp = player->GetComponent<AnimationComponent>();
        if (!transform || !animComp || !animComp->playing || !animComp->graphEnabled || animComp->graphStates.empty())
            return;

        const int stateIndex = glm::clamp(animComp->activeState, 0, static_cast<int>(animComp->graphStates.size()) - 1);
        const auto &state = animComp->graphStates[static_cast<size_t>(stateIndex)];

        if (!state.rootMotionEnabled || state.durationSeconds <= 0.0001f)
            return;

        glm::vec3 cycleDelta = state.rootTranslationDelta;
        if (!state.rootMotionAllowVertical)
            cycleDelta.y = 0.0f;

        const float cycleDistance = glm::length(cycleDelta);
        if (cycleDistance <= 0.0001f)
            return;

        // Normalise Mixamo centimetre-scale sources.
        const float unitScale = (cycleDistance > 15.0f) ? 0.01f : 1.0f;
        const float rootMotionScale = std::max(0.0f, state.rootMotionScale);
        glm::vec3 velocity = ((cycleDelta * unitScale) / state.durationSeconds) * rootMotionScale;

        const float maxSpeed = std::max(0.0f, state.rootMotionMaxSpeed);
        const float speed = glm::length(velocity);
        if (maxSpeed > 0.0f && speed > maxSpeed)
            velocity *= (maxSpeed / speed);

        transform->Position += velocity * (deltaTime * animComp->playbackSpeed);
    }

    void SandboxApp::ApplyThirdPersonCamera(float /*deltaTime*/)
    {
        Entity *player = m_Scene.FindEntityByID(m_PlayerEntityID);
        if (!player)
            return;

        auto *transform = player->GetComponent<TransformComponent>();
        if (!transform)
            return;

        auto &camera = m_Renderer.GetCamera();
        const glm::vec3 target = transform->Position + glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 camPos = target + m_ThirdPersonOffset;

        glm::vec3 lookDir = glm::normalize(target - camPos);
        float yaw = std::atan2(lookDir.z, lookDir.x);
        float pitch = std::asin(glm::clamp(lookDir.y, -1.0f, 1.0f));

        camera.SetEditorPosition(camPos);
        camera.SetYawPitch(yaw, pitch);
    }

    void SandboxApp::render()
    {
        m_Timer.Update();
        float deltaTime = m_Timer.GetDeltaTime();

        // Process async asset loads
        m_AssetLoader.ProcessCompletedLoads();

        // Non-ImGui loading indicator in window title.
        const int pending = m_PendingModelLoads.load();
        if (pending > 0)
        {
            std::ostringstream title;
            title << m_BaseTitle << " - Loading models: " << pending;
            const int failed = m_FailedModelLoads.load();
            if (failed > 0)
                title << " (failed: " << failed << ")";
            glfwSetWindowTitle(m_Window.getWindow(), title.str().c_str());
            m_LoadCompleteAnnounced = false;
        }
        else if (!m_LoadCompleteAnnounced)
        {
            std::ostringstream title;
            title << m_BaseTitle;
            const int failed = m_FailedModelLoads.load();
            if (failed > 0)
                title << " - Load complete (failed: " << failed << ")";
            else
                title << " - Load complete";
            glfwSetWindowTitle(m_Window.getWindow(), title.str().c_str());
            m_LoadCompleteAnnounced = true;
        }

        update(deltaTime);

        // Get the actual framebuffer size (accounts for high DPI)
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(m_Window.getWindow(), &fbWidth, &fbHeight);

        // Setup viewport to full framebuffer
        glViewport(0, 0, fbWidth, fbHeight);
        glEnable(GL_DEPTH_TEST);
        m_Renderer.clear(0.1f, 0.2f, 0.3f, 1.0f);

        // Render scene directly to the framebuffer
        static ::Shader shader("resources/shaders/basic.vert", "resources/shaders/basic.frag");

        // Render skybox/entities first, then grid overlay.
        m_Scene.Render(m_Renderer, shader);
        m_Scene.RenderGrid(m_Renderer);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        RenderHud();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_Window.getWindow());
    }

    void SandboxApp::RenderHud()
    {
        if (!m_ShowHud)
            return;

        std::string queuedScenePath;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 16.0f, viewport->WorkPos.y + 16.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.72f);

        constexpr ImGuiWindowFlags hudFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav;

        if (!ImGui::Begin("Sandbox HUD", nullptr, hudFlags))
        {
            ImGui::End();
            return;
        }

        const std::string sceneName = m_CurrentScenePath.empty()
                                          ? std::string("None")
                                          : std::filesystem::path(m_CurrentScenePath).filename().string();
        ImGui::Text("Scene: %s", sceneName.c_str());
        if (!m_AvailableScenes.empty() && m_CurrentSceneIndex >= 0)
            ImGui::Text("Scene Index: %d / %d", m_CurrentSceneIndex + 1, static_cast<int>(m_AvailableScenes.size()));

        if (!m_AvailableScenes.empty())
        {
            const char *currentSceneLabel = sceneName.c_str();
            if (ImGui::BeginCombo("Scene Picker", currentSceneLabel))
            {
                for (size_t i = 0; i < m_AvailableScenes.size(); ++i)
                {
                    const bool selected = (static_cast<int>(i) == m_CurrentSceneIndex);
                    const std::string optionLabel = std::filesystem::path(m_AvailableScenes[i]).filename().string();
                    if (ImGui::Selectable(optionLabel.c_str(), selected))
                        queuedScenePath = m_AvailableScenes[i];
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        Entity *player = m_Scene.FindEntityByID(m_PlayerEntityID);
        auto *animComp = player ? player->GetComponent<AnimationComponent>() : nullptr;
        const char *activeStateName = "None";
        if (animComp && animComp->activeState >= 0 && animComp->activeState < static_cast<int>(animComp->graphStates.size()))
            activeStateName = animComp->graphStates[static_cast<size_t>(animComp->activeState)].name.c_str();

        ImGui::Separator();
        ImGui::Text("Active State: %s", activeStateName);
        ImGui::Text("Playing: %s", (animComp && animComp->playing) ? "Yes" : "No");
        ImGui::Text("Root Motion: %s", m_UseRootMotion ? "On" : "Off");
        ImGui::Text("Camera: %s", m_ThirdPersonMode ? "Third Person" : "Free");

        if (player && animComp && animComp->graphEnabled)
        {
            std::set<std::string> uniqueTriggers;
            for (const auto &transition : animComp->graphTransitions)
            {
                if (!transition.trigger.empty())
                    uniqueTriggers.insert(transition.trigger);
            }

            if (!uniqueTriggers.empty())
            {
                ImGui::Separator();
                ImGui::Text("Graph Triggers");
                for (const auto &trigger : uniqueTriggers)
                {
                    if (ImGui::Button(trigger.c_str()))
                        ApplyGraphTrigger(player, trigger, true);
                    if (&trigger != &(*uniqueTriggers.rbegin()))
                        ImGui::SameLine();
                }
            }

            if (!animComp->inputBindings.empty())
            {
                ImGui::Separator();
                ImGui::Text("Input Bindings");
                for (const auto &binding : animComp->inputBindings)
                {
                    const bool pressed = (binding.key >= 0) && Input::IsKeyPressed(binding.key);
                    ImGui::Text("%s <- %s [%s]",
                                binding.trigger.c_str(),
                                KeyToLabel(binding.key),
                                pressed ? "DOWN" : "up");
                }
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("F1 HUD  [ / ] Scene  F5 Reload  T Camera  M Root Motion  P Play  R Reset");
        ImGui::End();

        if (!queuedScenePath.empty())
            LoadScene(queuedScenePath);
    }

    void SandboxApp::RefreshAvailableScenes()
    {
        m_AvailableScenes.clear();

        const std::filesystem::path scenesDir("scenes");
        if (!std::filesystem::exists(scenesDir))
            return;

        for (const auto &entry : std::filesystem::directory_iterator(scenesDir))
        {
            if (!entry.is_regular_file())
                continue;

            const std::string ext = entry.path().extension().string();
            if (ext != ".json" && ext != ".JSON")
                continue;

            m_AvailableScenes.push_back(entry.path().generic_string());
        }

        std::sort(m_AvailableScenes.begin(), m_AvailableScenes.end());

        if (!m_CurrentScenePath.empty())
        {
            auto it = std::find(m_AvailableScenes.begin(), m_AvailableScenes.end(), m_CurrentScenePath);
            m_CurrentSceneIndex = (it != m_AvailableScenes.end()) ? static_cast<int>(std::distance(m_AvailableScenes.begin(), it)) : -1;
        }

        if (m_CurrentSceneIndex < 0 && !m_AvailableScenes.empty())
            m_CurrentSceneIndex = 0;
    }

    void SandboxApp::LoadCurrentSceneFromList()
    {
        RefreshAvailableScenes();
        if (m_CurrentSceneIndex < 0 || m_CurrentSceneIndex >= static_cast<int>(m_AvailableScenes.size()))
        {
            std::cerr << "[SandboxApp] No scenes available in scenes/" << std::endl;
            return;
        }

        LoadScene(m_AvailableScenes[static_cast<size_t>(m_CurrentSceneIndex)]);
    }

    void SandboxApp::LoadNextScene()
    {
        RefreshAvailableScenes();
        if (m_AvailableScenes.empty())
        {
            std::cerr << "[SandboxApp] No scenes available in scenes/" << std::endl;
            return;
        }

        if (m_CurrentSceneIndex < 0)
            m_CurrentSceneIndex = 0;
        else
            m_CurrentSceneIndex = (m_CurrentSceneIndex + 1) % static_cast<int>(m_AvailableScenes.size());

        LoadCurrentSceneFromList();
    }

    void SandboxApp::LoadPreviousScene()
    {
        RefreshAvailableScenes();
        if (m_AvailableScenes.empty())
        {
            std::cerr << "[SandboxApp] No scenes available in scenes/" << std::endl;
            return;
        }

        if (m_CurrentSceneIndex < 0)
            m_CurrentSceneIndex = 0;
        else
            m_CurrentSceneIndex = (m_CurrentSceneIndex - 1 + static_cast<int>(m_AvailableScenes.size())) % static_cast<int>(m_AvailableScenes.size());

        LoadCurrentSceneFromList();
    }

    void SandboxApp::LoadScene(const std::string &scenePath)
    {
        std::cout << "[SandboxApp] Queueing scene load: " << scenePath << std::endl;
        m_CurrentScenePath = std::filesystem::path(scenePath).generic_string();
        RefreshAvailableScenes();
        m_PendingModelLoads = 0;
        m_FailedModelLoads = 0;
        m_LoadCompleteAnnounced = false;
        m_Scene.ClearScene();
        m_Scene.SetLightGizmoVisible(false);

        m_AssetLoader.LoadSceneAsync(&m_Scene, scenePath,
                                     [this, scenePath](bool success, const std::string &errorMsg)
                                     {
                                         if (!success)
                                         {
                                             std::cerr << "[SandboxApp] Failed to load scene '" << scenePath << "': " << errorMsg << std::endl;
                                             m_SceneLoaded = true; // avoid stalling
                                             return;
                                         }

                                         // Assign player to the first mesh entity. Keep other characters invisible.
                                         bool playerAssigned = false;
                                         size_t queuedModels = 0;
                                         for (const auto &entity : m_Scene.GetEntities())
                                         {
                                             auto *meshComp = entity->GetComponent<MeshComponent>();
                                             if (!meshComp || meshComp->ModelPath.empty())
                                                 continue;

                                             if (!playerAssigned)
                                             {
                                                 m_PlayerEntityID = entity->GetID();
                                                 playerAssigned = true;
                                             }
                                             else
                                             {
                                                 // Keep only one active character for input-driven animation switching.
                                                 meshComp->Visible = false;
                                                 continue;
                                             }

                                             // If AssetLoader preloaded the model it will already be attached.
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
                                                                                          mesh->ModelPtr = model;
                                                                                          SetupEntityAnimation(target, false);
                                                                                      }
                                                                                      else if (!loaded)
                                                                                      {
                                                                                          m_FailedModelLoads++;
                                                                                          std::cerr << "[SandboxApp] Failed async model load '" << modelPath << "': " << loadError << std::endl;
                                                                                      }
                                                                                  }
                                                                                  m_PendingModelLoads--;
                                                                              });
                                                 queuedModels++;
                                             }
                                         }

                                         m_SceneLoaded = true;
                                         const std::string sceneName = std::filesystem::path(m_CurrentScenePath).filename().string();
                                         std::cout << "[SandboxApp] Controls: F1 HUD, P play/pause, R reset, graph input keys from scene, T toggle third-person camera, M toggle root motion, [ previous scene, ] next scene, F5 reload scene." << std::endl;
                                         std::cout << "[SandboxApp] Active scene: " << sceneName;
                                         if (!m_AvailableScenes.empty() && m_CurrentSceneIndex >= 0)
                                             std::cout << " (" << (m_CurrentSceneIndex + 1) << "/" << m_AvailableScenes.size() << ")";
                                         std::cout << std::endl;
                                         std::cout << "[SandboxApp] Scene loaded. Queued " << queuedModels << " model(s) asynchronously." << std::endl;
                                     });
    }

    void SandboxApp::run()
    {
        m_Window.show();

        // Load default scene if not already loaded
        if (!m_SceneLoaded)
        {
            RefreshAvailableScenes();
            const auto defaultIt = std::find(m_AvailableScenes.begin(), m_AvailableScenes.end(), "scenes/default_animation_scene.json");
            if (defaultIt != m_AvailableScenes.end())
                m_CurrentSceneIndex = static_cast<int>(std::distance(m_AvailableScenes.begin(), defaultIt));
            else if (!m_AvailableScenes.empty())
                m_CurrentSceneIndex = 0;

            LoadCurrentSceneFromList();
        }

        while (!glfwWindowShouldClose(m_Window.getWindow()))
        {
            processEvents();
            render();
        }
    }
}
