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
    AppLayer::AppLayer(int width, int height, const char* title)
        : m_Window(width, height, title)
    {
        initImGui();
    }

    AppLayer::~AppLayer()
    {
        shutdownImGui();
    }

    void AppLayer::initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(m_Window.getWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        std::cout << std::filesystem::current_path() << std::endl;

        // --- Fonts ---
        io.FontDefault = io.Fonts->AddFontFromFileTTF(
            "resources/fonts/Roboto/Roboto-VariableFont_wdth,wght.ttf",
            14.0f
        );

        io.Fonts->AddFontFromFileTTF(
            "resources/fonts/Roboto_Mono/RobotoMono-VariableFont_wght.ttf",
            13.0f
        );

        // If fonts are added after backend init, force rebuild:
        io.Fonts->Build();
    }


    void AppLayer::shutdownImGui()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void AppLayer::processEvents()
    {
        m_Window.pollEvents();
    }

    void AppLayer::render()
    {
        // Clear screen
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Platform windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_context);
        }

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