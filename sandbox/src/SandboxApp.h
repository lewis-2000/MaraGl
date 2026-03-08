#pragma once
#include "Window.h"
#include "Renderer.h"
#include "Scene.h"
#include "Timer.h"
#include "Input.h"
#include "Shader.h"
#include "AssetLoader.h"
#include <memory>

namespace MaraGl
{
    class SandboxApp
    {
    public:
        SandboxApp(int width = 1366, int height = 768, const char *title = "MaraGl Sandbox");
        ~SandboxApp();

        void run();
        void LoadScene(const std::string &scenePath);

    private:
        void processEvents();
        void update(float deltaTime);
        void render();

        Window m_Window;
        Renderer m_Renderer;
        Scene m_Scene;
        Timer m_Timer;
        AssetLoader m_AssetLoader;
        bool m_SceneLoaded = false;
    };
}
