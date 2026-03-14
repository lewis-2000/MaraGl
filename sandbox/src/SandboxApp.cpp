#include "SandboxApp.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "AnimationComponent.h"
#include "Animator.h"
#include "Model.h"
#include "SceneSerializer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <sstream>
#include <iostream>
#include <cmath>
#include <unordered_map>
#include <algorithm>
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
                animComp->currentAnimation = resolvedIndex;
                const auto &clip = animComp->animations[static_cast<size_t>(resolvedIndex)];
                state.durationSeconds = (clip.ticksPerSecond > 0.0f)
                                            ? (clip.duration / clip.ticksPerSecond)
                                            : 0.0f;
            }
        }

        void SetupEntityAnimation(Entity *entity, bool autoPlay)
        {
            if (!entity)
                return;

            auto *meshComp = entity->GetComponent<MeshComponent>();
            if (!meshComp || !meshComp->ModelPtr || !meshComp->ModelPtr->HasAnimations())
                return;

            AnimationComponent *animComp = entity->GetComponent<AnimationComponent>();
            if (!animComp)
                animComp = &entity->AddComponent<AnimationComponent>();

            animComp->animations = meshComp->ModelPtr->LoadAnimations();
            animComp->boneInfoMap = meshComp->ModelPtr->GetBoneInfoMap();

            const int boneCount = meshComp->ModelPtr->GetBoneCount();
            animComp->boneTransforms.assign(static_cast<size_t>(boneCount), glm::mat4(1.0f));

            if (animComp->animations.empty())
                return;

            const bool hasGraphState = animComp->graphEnabled && !animComp->graphStates.empty();
            if (hasGraphState)
            {
                animComp->activeState = glm::clamp(animComp->activeState, 0, static_cast<int>(animComp->graphStates.size()) - 1);
                ResolveStateAnimation(animComp, animComp->activeState);
                animComp->looping = animComp->graphStates[static_cast<size_t>(animComp->activeState)].loop;
            }
            else
            {
                animComp->currentAnimation = 0;
                animComp->looping = true;
            }

            animComp->currentTime = 0.0f;
            animComp->playbackSpeed = 1.0f;
            animComp->playing = autoPlay;

            const aiScene *scene = meshComp->ModelPtr->GetScene();
            if (scene && scene->mRootNode && (!animComp->graphEnabled || hasGraphState))
            {
                glm::mat4 identity(1.0f);
                glm::mat4 globalInverseTransform = Animator::GetGlobalInverseTransform(scene);
                Animator::CalculateBoneTransform(animComp, animComp->animations[static_cast<size_t>(animComp->currentAnimation)], scene->mRootNode, identity, globalInverseTransform);
            }
        }

        bool ApplyGraphTrigger(Entity *entity, const std::string &trigger, bool autoPlay)
        {
            if (!entity)
                return false;

            auto *animComp = entity->GetComponent<AnimationComponent>();
            if (!animComp || !animComp->graphEnabled || animComp->graphStates.empty())
                return false;

            for (const auto &transition : animComp->graphTransitions)
            {
                if (transition.fromState != animComp->activeState || transition.trigger != trigger)
                    continue;

                if (transition.toState < 0 || transition.toState >= static_cast<int>(animComp->graphStates.size()))
                    return false;

                animComp->activeState = transition.toState;
                ResolveStateAnimation(animComp, animComp->activeState);
                animComp->looping = animComp->graphStates[static_cast<size_t>(animComp->activeState)].loop;
                animComp->currentTime = 0.0f;
                animComp->playing = autoPlay;

                return true;
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

        std::cout << "[SandboxApp] Initialized" << std::endl;
    }

    SandboxApp::~SandboxApp()
    {
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
            animComp->currentTime = 0.0f;

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

        glfwSwapBuffers(m_Window.getWindow());
    }

    void SandboxApp::LoadScene(const std::string &scenePath)
    {
        std::cout << "[SandboxApp] Loading scene: " << scenePath << std::endl;
        m_PendingModelLoads = 0;
        m_FailedModelLoads = 0;
        m_LoadCompleteAnnounced = false;
        m_Scene.ClearScene();
        m_Scene.SetLightGizmoVisible(false);

        nlohmann::json sceneJson;
        if (!SceneSerializer::ParseSceneFile(scenePath, sceneJson))
        {
            std::cerr << "[SandboxApp] Failed to load scene: " << scenePath << std::endl;
            return;
        }

        // Apply entities/settings immediately, but skip heavyweight model loads.
        if (!SceneSerializer::ApplySceneData(&m_Scene, sceneJson, false))
        {
            std::cerr << "[SandboxApp] Failed to apply scene data: " << scenePath << std::endl;
            return;
        }

        // Stream models asynchronously so the app remains responsive.
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

            const uint32_t entityID = entity->GetID();
            const std::string modelPath = meshComp->ModelPath;
            m_PendingModelLoads++;

            m_AssetLoader.LoadModelAsync(modelPath, std::to_string(entityID),
                                         [this, entityID, modelPath](std::shared_ptr<Model> model, bool success, const std::string &errorMsg)
                                         {
                                             Entity *target = m_Scene.FindEntityByID(entityID);
                                             if (target)
                                             {
                                                 auto *mesh = target->GetComponent<MeshComponent>();
                                                 if (mesh && success && model)
                                                 {
                                                     mesh->ModelPtr = model;
                                                     SetupEntityAnimation(target, false);
                                                 }
                                                 else if (!success)
                                                 {
                                                     m_FailedModelLoads++;
                                                     std::cerr << "[SandboxApp] Failed async model load '" << modelPath << "': " << errorMsg << std::endl;
                                                 }
                                             }

                                             m_PendingModelLoads--;
                                         });
            queuedModels++;
        }

        m_SceneLoaded = true;
        std::cout << "[SandboxApp] Controls: P play/pause, R reset, graph input keys from scene, T toggle third-person camera, M toggle root motion." << std::endl;
        std::cout << "[SandboxApp] Scene loaded. Queued " << queuedModels << " model(s) asynchronously." << std::endl;
    }

    void SandboxApp::run()
    {
        // Load default scene if not already loaded
        if (!m_SceneLoaded)
        {
            LoadScene("scenes/default_animation_scene.json");
        }

        while (!glfwWindowShouldClose(m_Window.getWindow()))
        {
            processEvents();
            render();
        }
    }
}
