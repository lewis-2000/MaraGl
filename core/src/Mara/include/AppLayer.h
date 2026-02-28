#pragma once
#include "Window.h"
#include "VBO.h"
#include "VAO.h"
#include "Shader.h"

#include "Renderer.h"
#include "ImGuiLayer.h"
#include "Framebuffer.h"

namespace MaraGl
{
    class AppLayer
    {
    public:
        AppLayer(int width = 1280, int height = 720, const char *title = "MaraGl App");
        ~AppLayer();

        void run(); // main loop

    private:
        // void initImGui();
        // void shutdownImGui();
        void processEvents();
        void render();

    private:
        Window m_Window;
        Renderer m_Renderer;
        ImGuiLayer m_ImGuiLayer;
        Framebuffer m_Framebuffer;
    };
}