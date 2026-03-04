#include "include/AppLayer.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>

namespace MaraGl
{

    AppLayer::AppLayer(int width, int height, const char *title)
        : m_Window(width, height, title),
          m_Renderer(),
          m_ImGuiLayer(m_Window),
          m_Framebuffer(width, height)
    {
        Input::Init(m_Window.getWindow());
    }

    AppLayer::~AppLayer()
    {
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

        // Always update input first, before ImGui
        Input::Update();

        // Update camera - only process input if ScenePanel is focused
        ScenePanel *scenePanel = m_ImGuiLayer.GetScenePanel();
        bool sceneFocused = scenePanel ? scenePanel->IsFocused() : false;

        auto &camera = m_Renderer.GetCamera();
        camera.Update(deltaTime, sceneFocused);

        // Compile once, reuse every frame
        static Shader shader("resources/shaders/basic.vert", "resources/shaders/basic.frag");

        // 1) Render scene into offscreen framebuffer
        m_Framebuffer.bind();
        glViewport(0, 0, m_Framebuffer.getWidth(), m_Framebuffer.getHeight());
        glEnable(GL_DEPTH_TEST);
        m_Renderer.clear(0.1f, 0.2f, 0.3f, 1.0f);

        // Only render the model if it's been loaded
        if (m_ModelLoaded && m_Model)
        {
            m_Renderer.DrawModel(*m_Model, shader);
        }

        m_Framebuffer.unbind();

        // 2) Render ImGui to default framebuffer
        int ww = 0, hh = 0;
        glfwGetFramebufferSize(m_Window.getWindow(), &ww, &hh);
        glViewport(0, 0, ww, hh);

        m_ImGuiLayer.begin();
        m_ImGuiLayer.renderDockspace(&m_Framebuffer, &m_Renderer);
        m_ImGuiLayer.end();

        glfwSwapBuffers(m_Window.getWindow());

        // Check if user requested a model load from ModelLoaderPanel
        ModelLoaderPanel *loaderPanel = m_ImGuiLayer.GetModelLoaderPanel();
        if (loaderPanel && loaderPanel->ShouldLoadModel())
        {
            std::string modelPath = loaderPanel->GetSelectedModelPath();
            try
            {
                m_Model = std::make_unique<Model>(modelPath);
                m_ModelLoaded = true;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error loading model from: " << modelPath << "\n"
                          << e.what() << std::endl;
            }
        }

        // Auto-load default model AFTER ImGui has rendered for 2 frames (window is now visible)
        m_FrameCount++;
        if (!m_ModelLoaded && m_FrameCount >= 2)
        {
            m_Model = std::make_unique<Model>("resources/models/Tree/trees9.obj");
            m_ModelLoaded = true;
        }
    }

    void AppLayer::run()
    {
        while (!glfwWindowShouldClose(m_Window.getWindow()))
        {
            processEvents();
            render();
        }
    }
}