#include "SandboxApp.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "SceneSerializer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>

namespace MaraGl
{
    SandboxApp::SandboxApp(int width, int height, const char *title)
        : m_Window(width, height, title),
          m_Renderer(),
          m_Scene()
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

        // Draw grid and all scene entities
        m_Scene.RenderGrid(m_Renderer);
        m_Scene.Render(m_Renderer, shader);

        glfwSwapBuffers(m_Window.getWindow());
    }

    void SandboxApp::LoadScene(const std::string &scenePath)
    {
        std::cout << "[SandboxApp] Loading scene: " << scenePath << std::endl;
        if (m_Scene.LoadFromFile(scenePath))
        {
            m_SceneLoaded = true;
            std::cout << "[SandboxApp] Scene loaded successfully" << std::endl;
        }
        else
        {
            std::cerr << "[SandboxApp] Failed to load scene: " << scenePath << std::endl;
        }
    }

    void SandboxApp::run()
    {
        // Load default scene if not already loaded
        if (!m_SceneLoaded)
        {
            LoadScene("scenes/scene1.json");
        }

        while (!glfwWindowShouldClose(m_Window.getWindow()))
        {
            processEvents();
            render();
        }
    }
}
