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
    }

    AppLayer::~AppLayer()
    {
    }

    void AppLayer::processEvents()
    {
        m_Window.pollEvents();
    }

    void AppLayer::render()
    {
        // Render scene to framebuffer
        m_Renderer.drawToFramebuffer(m_Framebuffer);

        m_ImGuiLayer.begin();

        // Pass framebuffer pointer to ImGuiLayer
        m_ImGuiLayer.renderDockspace(&m_Framebuffer);

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