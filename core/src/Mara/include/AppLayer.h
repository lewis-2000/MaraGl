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
#include "Scene.h"

#include "Model.h"
#include "ModelLoaderPanel.h"
#include "AssetLoader.h"
#include <memory>

namespace MaraGl
{
    class AppLayer
    {
    public:
        AppLayer(int width = 1280, int height = 720, const char *title = "MaraGl App");
        ~AppLayer();

        void run(); // main loop

        void LoadSkybox(const std::string &path);

    private:
        void processEvents();
        void render();
        Window m_Window;
        Renderer m_Renderer;
        ImGuiLayer m_ImGuiLayer;
        Framebuffer m_Framebuffer;
        Input m_Input;
        Timer m_Timer;
        PerspectiveCamera m_Camera;
        Scene m_Scene;
        AssetLoader m_AssetLoader;
        bool m_ModelLoaded = false;
        int m_FrameCount = 0;
    };
}