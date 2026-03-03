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

        Input::Update();

        auto &camera = m_Renderer.GetCamera();
        float speed = 5.0f * deltaTime;

        if (Input::IsKeyPressed(GLFW_KEY_W))
            camera.MoveForward(speed);
        if (Input::IsKeyPressed(GLFW_KEY_S))
            camera.MoveForward(-speed);
        if (Input::IsKeyPressed(GLFW_KEY_A))
            camera.MoveRight(-speed);
        if (Input::IsKeyPressed(GLFW_KEY_D))
            camera.MoveRight(speed);

        camera.Rotate(Input::GetMouseDeltaX() * 0.1f, Input::GetMouseDeltaY() * 0.1f);
        camera.Update(deltaTime);

        // Compile once, reuse every frame
        static Shader shader("resources/shaders/basic.vert", "resources/shaders/basic.frag");

        // 1) Render scene into offscreen framebuffer
        m_Framebuffer.bind();
        glViewport(0, 0, m_Framebuffer.getWidth(), m_Framebuffer.getHeight());
        glEnable(GL_DEPTH_TEST);
        m_Renderer.clear(0.1f, 0.2f, 0.3f, 1.0f);
        m_Renderer.DrawModel(m_Model, shader);
        m_Framebuffer.unbind();

        // 2) Render ImGui to default framebuffer
        int ww = 0, hh = 0;
        glfwGetFramebufferSize(m_Window.getWindow(), &ww, &hh);
        glViewport(0, 0, ww, hh);

        m_ImGuiLayer.begin();
        m_ImGuiLayer.renderDockspace(&m_Framebuffer, &m_Renderer);
        m_ImGuiLayer.end();

        glfwSwapBuffers(m_Window.getWindow());
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