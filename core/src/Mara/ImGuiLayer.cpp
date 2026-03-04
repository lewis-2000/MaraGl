#include "ImGuiLayer.h"
#include "Renderer.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <GLFW/glfw3.h>

namespace MaraGl
{
    // Constructor: Initializes ImGui layer with a reference to the main window
    ImGuiLayer::ImGuiLayer(Window &window)
        : m_Window(window)
    {
        init();
    }

    // Destructor: Cleans up ImGui resources
    ImGuiLayer::~ImGuiLayer()
    {
        shutdown();
    }

    // Initializes ImGui context, backends (GLFW + OpenGL3), loads fonts, and creates panels
    // Called during construction
    void ImGuiLayer::init()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        // Enable keyboard navigation (Tab/Arrows/Enter to navigate UI)
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Enable docking: allows windows to be docked and collapsed
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // Enable viewports: allows windows to float outside the main window
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // ImGui::StyleColorsDark();
        ImGui::StyleColorsLight();

        // Initialize GLFW and OpenGL3 backends for rendering ImGui
        ImGui_ImplGlfw_InitForOpenGL(m_Window.getWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        // --- Fonts ---
        // Load Roboto font as default (used for UI text)
        io.FontDefault = io.Fonts->AddFontFromFileTTF(
            "resources/fonts/Roboto/Roboto-VariableFont_wdth,wght.ttf",
            20.0f);

        // Load Roboto Mono font (used for code/console text)
        io.Fonts->AddFontFromFileTTF(
            "resources/fonts/Roboto_Mono/RobotoMono-VariableFont_wght.ttf",
            20.0f);

        // IMPORTANT: After adding fonts, must rebuild the font atlas for rendering
        io.Fonts->Build();
    }

    // Shuts down ImGui and cleans up backend resources
    // Called during destruction
    void ImGuiLayer::shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    // Begins a new ImGui frame - call at the start of each render loop
    // Sets up the drawing context for all UI elements
    void ImGuiLayer::begin()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // Ends the ImGui frame and renders all UI to the screen - call at the end of each render loop
    // Handles rendering to both main and docked viewports
    void ImGuiLayer::end()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO &io = ImGui::GetIO();
        // If viewports are enabled, render windows that exist outside the main window
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }
    }

    void ImGuiLayer::renderDockspace(Framebuffer *framebuffer, Renderer *renderer)
    {
        // Initialize panels on first call when we have a valid framebuffer
        if (!m_PanelsInitialized)
        {
            m_Framebuffer = framebuffer;
            m_PanelManager.AddPanel<InspectorPanel>();
            m_ScenePanel = m_PanelManager.AddPanel<ScenePanel>(m_Framebuffer);
            m_PanelManager.AddPanel<HierarchyPanel>();
            m_ModelLoaderPanel = m_PanelManager.AddPanel<ModelLoaderPanel>();
            // m_PanelManager.AddPanel<ConsolePanel>();
            m_PanelManager.AddPanel<EditorTimelinePanel>();
            // m_PanelManager.AddPanel<AssetsPanel>();
            m_PanelsInitialized = true;
        }
        else if (framebuffer != m_Framebuffer && m_ScenePanel)
        {
            // Update framebuffer if it changed
            m_Framebuffer = framebuffer;
            m_ScenePanel->SetFramebuffer(m_Framebuffer);
        }

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::SetNextWindowSizeConstraints(ImVec2(200, 150), ImVec2(FLT_MAX, FLT_MAX));

        ImGuiWindowFlags dockspace_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_MenuBar;

        ImGui::Begin("EditorDockspace", nullptr, dockspace_flags);

        ImGuiID dockspaceID = ImGui::GetID("MainDockspace");

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGui::DockSpace(dockspaceID, ImVec2(0, 0));

            static bool firstTime = true;
            if (firstTime)
            {
                firstTime = false;

                ImGui::DockBuilderRemoveNode(dockspaceID);
                ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->WorkSize);

                // --- Split main dockspace into left (Inspector+Hierarchy) and right (Scene+Tabs) ---
                ImGuiID dock_left = ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.25f, nullptr, &dockspaceID);
                ImGuiID dock_right = dockspaceID;

                // --- Split left column vertically: Inspector / Hierarchy ---
                ImGuiID dock_inspector = ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Up, 0.5f, nullptr, &dock_left);
                ImGuiID dock_hierarchy = dock_left;

                // --- Split right column vertically: Scene / Bottom Tabs ---
                ImGuiID dock_scene = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.9f, nullptr, &dock_right);
                ImGuiID dock_bottom_tabs = dock_right;

                // --- Dock windows to nodes ---
                ImGui::DockBuilderDockWindow("Inspector", dock_inspector);
                ImGui::DockBuilderDockWindow("Hierarchy", dock_hierarchy);
                ImGui::DockBuilderDockWindow("Scene", dock_scene);
                ImGui::DockBuilderDockWindow("Model Loader", dock_bottom_tabs);
                ImGui::DockBuilderDockWindow("Console", dock_bottom_tabs);
                ImGui::DockBuilderDockWindow("Assets", dock_bottom_tabs);
                ImGui::DockBuilderDockWindow("Timeline", dock_bottom_tabs);

                ImGui::DockBuilderFinish(dockspaceID);
            }
        }

        // --- Render all panels via PanelManager ---
        m_PanelManager.RenderPanels();

        ImGui::End();
    }
}