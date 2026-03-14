#pragma once
#include "Window.h"
#include "Renderer.h"
#include "Scene.h"
#include "Timer.h"
#include "Input.h"
#include "Shader.h"
#include "AssetLoader.h"
#include <glm/glm.hpp>
#include <array>
#include <atomic>
#include <memory>
#include <string>

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
        void HandleAnimationInput();
        void ApplyRootMotion(float deltaTime);
        void ApplyThirdPersonCamera(float deltaTime);
        bool WasKeyPressedOnce(int key);

        Window m_Window;
        Renderer m_Renderer;
        Scene m_Scene;
        Timer m_Timer;
        AssetLoader m_AssetLoader;
        bool m_SceneLoaded = false;
        std::string m_BaseTitle;
        std::atomic<int> m_PendingModelLoads{0};
        std::atomic<int> m_FailedModelLoads{0};
        bool m_LoadCompleteAnnounced = false;

        uint32_t m_PlayerEntityID = 0;
        bool m_ThirdPersonMode = true;
        bool m_UseRootMotion = true;
        glm::vec3 m_ThirdPersonOffset = glm::vec3(0.0f, 2.2f, 5.5f);
        std::array<bool, GLFW_KEY_LAST + 1> m_PreviousKeyState{};
    };
}
