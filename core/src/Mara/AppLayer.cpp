#include "include/AppLayer.h"
#include "Scene.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "Model.h"
#include "Shader.h"
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

        // Initialize scene with test entities
        Entity &entity1 = m_Scene.CreateEntity("TestEntity1");
        auto &nameComp1 = entity1.AddComponent<NameComponent>();
        nameComp1.Name = "TestEntity1";
        entity1.AddComponent<TransformComponent>();

        Entity &entity2 = m_Scene.CreateEntity("TestEntity2");
        auto &nameComp2 = entity2.AddComponent<NameComponent>();
        nameComp2.Name = "TestEntity2";
        auto &transform = entity2.AddComponent<TransformComponent>();
        transform.Position = glm::vec3(2.0f, 0.0f, 0.0f);

        // Pass scene to ImGuiLayer
        m_ImGuiLayer.SetScene(&m_Scene);
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

        // Update scene and panels
        m_Scene.Update(deltaTime);
        m_ImGuiLayer.Update(deltaTime);

        // Update camera - only process input if ScenePanel is focused
        ScenePanel *scenePanel = m_ImGuiLayer.GetScenePanel();
        bool sceneFocused = scenePanel ? scenePanel->IsFocused() : false;

        auto &camera = m_Renderer.GetCamera();
        camera.Update(deltaTime, sceneFocused);

        // Compile once, reuse every frame
        static ::Shader shader("resources/shaders/basic.vert", "resources/shaders/basic.frag");

        auto createModelEntity = [this](const std::string &modelPath, const std::string &fallbackName) -> bool
        {
            auto model = std::make_shared<Model>(modelPath);

            std::filesystem::path path(modelPath);
            std::string entityName = path.stem().string();
            if (entityName.empty())
                entityName = fallbackName;

            Entity &entity = m_Scene.CreateEntity(entityName);
            auto &nameComp = entity.AddComponent<NameComponent>();
            nameComp.Name = entityName;
            entity.AddComponent<TransformComponent>();
            auto &meshComp = entity.AddComponent<MeshComponent>();
            meshComp.ModelPtr = model;

            if (auto *hierarchyPanel = m_ImGuiLayer.GetHierarchyPanel())
                hierarchyPanel->SetSelectedEntityID(entity.GetID());

            std::cout << "Loaded model into entity: " << entityName << std::endl;
            return true;
        };

        // Load requests are handled before rendering so all panels reflect new entities immediately.
        ModelLoaderPanel *loaderPanel = m_ImGuiLayer.GetModelLoaderPanel();
        if (loaderPanel && loaderPanel->ShouldLoadModel())
        {
            std::string modelPath = loaderPanel->GetSelectedModelPath();
            try
            {
                createModelEntity(modelPath, "ModelEntity");
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error loading model from: " << modelPath << "\n"
                          << e.what() << std::endl;
            }
        }

        // Auto-load default model after startup.
        m_FrameCount++;
        if (!m_ModelLoaded && m_FrameCount >= 2)
        {
            try
            {
                m_ModelLoaded = createModelEntity("resources/models/Tree/trees9.obj", "DefaultTree");
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error loading default model: " << e.what() << std::endl;
            }
        }

        // 1) Render scene into offscreen framebuffer
        m_Framebuffer.bind();
        glViewport(0, 0, m_Framebuffer.getWidth(), m_Framebuffer.getHeight());
        glEnable(GL_DEPTH_TEST);
        m_Renderer.clear(0.1f, 0.2f, 0.3f, 1.0f);

        // Render all entities in the scene with the ECS
        m_Scene.Render(m_Renderer, shader);

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

    void AppLayer::LoadSkybox(const std::string &path)
    {
        m_Scene.LoadSkybox(path);
    }
}