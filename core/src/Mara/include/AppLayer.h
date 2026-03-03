#pragma once
#include "Window.h"
#include "VBO.h"
#include "VAO.h"
#include "Shader.h"

#include "Renderer.h"
#include "ImGuiLayer.h"
#include "Framebuffer.h"
#include "Input.h"
#include "PerspectiveCamera.h"
#include "Timer.h"

#include "Model.h"

namespace MaraGl
{
    class AppLayer
    {
    public:
        AppLayer(int width = 1280, int height = 720, const char *title = "MaraGl App");
        ~AppLayer();

        void run(); // main loop

    private:
        void processEvents();
        void render();

    private:
        Window m_Window;
        Renderer m_Renderer;
        ImGuiLayer m_ImGuiLayer;
        Framebuffer m_Framebuffer;
        Input m_Input;
        Timer m_Timer;
        PerspectiveCamera m_Camera;
        Model m_Model{"resources/models/Tree/trees9.obj"};
    };
}