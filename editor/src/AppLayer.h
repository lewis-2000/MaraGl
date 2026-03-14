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
#include <atomic>
#include <array>
#include <memory>
#include <GLFW/glfw3.h>

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
        void LoadScene(const std::string &scenePath);
        void HandleGraphInput(bool sceneFocused);
        void UpdateCameraBehindModel();
        void ApplyRootMotionInGameMode(float deltaTime);
        bool WasKeyPressedOnce(int key);
        void QueueGraphModelLoad(uint32_t entityID, const std::string &modelPath, bool autoPlayAfterLoad);
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
        bool m_SceneLoaded = false;
        bool m_GameMode = false;
        std::atomic<int> m_PendingModelLoads{0};
        std::atomic<int> m_FailedModelLoads{0};
        uint32_t m_GraphEntityID = 0;
        std::array<bool, GLFW_KEY_LAST + 1> m_PreviousKeyState{};
    };
}
