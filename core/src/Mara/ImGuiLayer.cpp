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
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();
        // ImGui::StyleColorsClassic();

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

        ApplyModernEditorStyle(); // Apply custom styling for a modern editor look
    }

    void ImGuiLayer::ApplyModernEditorStyle()
    {
        ImGuiStyle &style = ImGui::GetStyle();

        // ---- Rounding ----
        style.WindowRounding = 6.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 5.0f;

        // ---- Spacing ----
        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(8, 6);
        style.ItemInnerSpacing = ImVec2(6, 4);
        style.IndentSpacing = 18.0f;
        style.ScrollbarSize = 14.0f;

        // ---- Borders ----
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.FrameRounding = 3.0f;
        style.TabBorderSize = 0.0f;
        style.ChildBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;

        auto &c = style.Colors;

        // ---- Backgrounds ----
        c[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.13f, 1.0f);
        c[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
        c[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);

        // ---- Headers / Selection ----
        c[ImGuiCol_Header] = ImVec4(0.24f, 0.24f, 0.28f, 1.0f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.32f, 0.38f, 1.0f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.36f, 0.42f, 1.0f);

        // ---- Buttons ----
        c[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.34f, 1.0f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.42f, 1.0f);

        // ---- Tabs ----
        c[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
        c[ImGuiCol_TabHovered] = ImVec4(0.32f, 0.32f, 0.38f, 1.0f);
        c[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.34f, 1.0f);
        c[ImGuiCol_TabUnfocused] = c[ImGuiCol_Tab];
        c[ImGuiCol_TabUnfocusedActive] = c[ImGuiCol_TabActive];

        // ---- Title bar ----
        c[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        c[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);

        // ---- Docking ----
        c[ImGuiCol_DockingPreview] = ImVec4(0.40f, 0.60f, 1.00f, 0.25f);
        c[ImGuiCol_DockingEmptyBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);

        // ---- Scrollbar ----
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.24f, 0.28f, 1.0f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.36f, 1.0f);
        c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.42f, 1.0f);

        // ---- Checkmarks ----
        c[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.60f, 1.00f, 1.0f);
        c[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.60f, 1.00f, 1.0f);
        c[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.70f, 1.00f, 1.0f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.70f, 1.00f, 1.0f);

        // ---- Text ----
        c[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
        c[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
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
            m_InspectorPanel = m_PanelManager.AddPanel<InspectorPanel>();
            m_ScenePanel = m_PanelManager.AddPanel<ScenePanel>(m_Framebuffer);
            m_HierarchyPanel = m_PanelManager.AddPanel<HierarchyPanel>();
            m_ModelLoaderPanel = m_PanelManager.AddPanel<ModelLoaderPanel>();
            m_TimelinePanel = m_PanelManager.AddPanel<EditorTimelinePanel>();
            m_SceneSettingsPanel = m_PanelManager.AddPanel<SceneSettingsPanel>();
            // m_PanelManager.AddPanel<ConsolePanel>();
            // m_PanelManager.AddPanel<AssetsPanel>();
            m_PanelsInitialized = true;

            // Apply scene pointer to newly created panels
            if (m_Scene)
            {
                if (m_InspectorPanel)
                {
                    m_InspectorPanel->SetScene(m_Scene);
                    m_InspectorPanel->SetHierarchyPanel(m_HierarchyPanel);
                }
                if (m_HierarchyPanel)
                    m_HierarchyPanel->SetScene(m_Scene);
                if (m_TimelinePanel)
                {
                    m_TimelinePanel->SetScene(m_Scene);
                    m_TimelinePanel->SetHierarchyPanel(m_HierarchyPanel);
                }
                if (m_SceneSettingsPanel)
                    m_SceneSettingsPanel->SetScene(m_Scene);
            }
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
                ImGuiID dock_left = ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.40f, nullptr, &dockspaceID);
                ImGuiID dock_right = dockspaceID;

                // --- Split left column vertically: Inspector / Hierarchy ---
                ImGuiID dock_inspector = ImGui::DockBuilderSplitNode(dock_left, ImGuiDir_Up, 0.55f, nullptr, &dock_left);
                ImGuiID dock_hierarchy = dock_left;

                // --- Split right column vertically: Scene / Bottom Tabs ---
                ImGuiID dock_scene = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Up, 0.75f, nullptr, &dock_right);
                ImGuiID dock_bottom_tabs = dock_right;

                // --- Dock windows to nodes ---
                ImGui::DockBuilderDockWindow("Inspector", dock_inspector);
                ImGui::DockBuilderDockWindow("Hierarchy", dock_hierarchy);
                ImGui::DockBuilderDockWindow("Scene", dock_scene);
                ImGui::DockBuilderDockWindow("Scene Settings", dock_bottom_tabs);
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

    void ImGuiLayer::SetScene(Scene *scene)
    {
        m_Scene = scene;

        // Only apply to panels if they've been initialized
        if (!m_PanelsInitialized)
            return;

        if (m_InspectorPanel)
        {
            m_InspectorPanel->SetScene(scene);
            m_InspectorPanel->SetHierarchyPanel(m_HierarchyPanel);
        }
        if (m_HierarchyPanel)
            m_HierarchyPanel->SetScene(scene);
        if (m_TimelinePanel)
        {
            m_TimelinePanel->SetScene(scene);
            m_TimelinePanel->SetHierarchyPanel(m_HierarchyPanel);
        }
        if (m_SceneSettingsPanel)
            m_SceneSettingsPanel->SetScene(scene);
    }
}