#include "SandboxApp.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "SceneSerializer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>

namespace MaraGl
{
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
        camera.Update(deltaTime, true); // Always focus on scene

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
        size_t queuedModels = 0;
        for (const auto &entity : m_Scene.GetEntities())
        {
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
        std::cout << "[SandboxApp] Scene loaded. Queued " << queuedModels << " model(s) asynchronously." << std::endl;
    }

    void SandboxApp::run()
    {
        // Load default scene if not already loaded
        if (!m_SceneLoaded)
        {
            LoadScene("scenes/scene3.json");
        }

        while (!glfwWindowShouldClose(m_Window.getWindow()))
        {
            processEvents();
            render();
        }
    }
}
