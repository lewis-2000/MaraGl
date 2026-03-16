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
            auto &targetState = animComp->graphStates[static_cast<size_t>(animComp->activeState)];

            ResolveStateAnimation(animComp, animComp->activeState);
            animComp->looping = targetState.loop;
            animComp->currentTime = 0.0f;
            animComp->playing = autoPlay;

            // Blend is currently stored for authoring/serialization; runtime blend support can be layered next.
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
        LoadScene("scenes/default_animation_scene.json");
    }

    AppLayer::~AppLayer()
    {
        Mesh::ClearTextureCache();
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
        m_ImGuiLayer.renderDockspace(&m_Framebuffer, &m_Renderer);
        m_ImGuiLayer.end();

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
        std::cout << "[AppLayer] Loading default scene: " << scenePath << std::endl;
        m_PendingModelLoads = 0;
        m_FailedModelLoads = 0;
        m_GraphEntityID = 0;
        m_Scene.ClearScene();
        m_Scene.SetLightGizmoVisible(true);

        nlohmann::json sceneJson;
        if (!SceneSerializer::ParseSceneFile(scenePath, sceneJson))
        {
            std::cerr << "[AppLayer] Failed to parse scene: " << scenePath << std::endl;
            return;
        }

        if (!SceneSerializer::ApplySceneData(&m_Scene, sceneJson, false))
        {
            std::cerr << "[AppLayer] Failed to apply scene data: " << scenePath << std::endl;
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
                                                     std::cerr << "[AppLayer] Failed async model load '" << modelPath << "': " << errorMsg << std::endl;
                                                 }
                                             }

                                             m_PendingModelLoads--;
                                         });
        }

        m_SceneLoaded = true;
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
