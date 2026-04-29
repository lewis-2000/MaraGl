#include "ImGuiLayer.h"
#include "ImGuiFontSetup.h"
#include "Renderer.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <imnodes.h>
#include <GLFW/glfw3.h>
#include <iostream>

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
        ImNodes::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        // Enable keyboard navigation (Tab/Arrows/Enter to navigate UI)
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Enable docking: allows windows to be docked and collapsed
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // Enable viewports: allows windows to float outside the main window
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        // Let ImGui adapt fonts and viewport scale to monitor DPI.
        io.ConfigDpiScaleFonts = true;
        io.ConfigDpiScaleViewports = true;

        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();
        // ImGui::StyleColorsClassic();

        // Initialize GLFW and OpenGL3 backends for rendering ImGui
        ImGui_ImplGlfw_InitForOpenGL(m_Window.getWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        const ImGuiFontSet fonts = ConfigureDefaultImGuiFonts(io, 16.0f, 15.0f);
        if (fonts.mergedIconFont)
        {
            m_IconFonts["FontAwesome"] = fonts.mergedIconFont;
            std::cout << "[ImGuiLayer] Merged FontAwesome icon glyphs from: " << fonts.mergedIconFontPath << std::endl;
        }
        else
        {
            LoadDefaultIconFonts();
        }

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
        ImNodes::DestroyContext();
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
            m_AnimationGraphPanel = m_PanelManager.AddPanel<AnimationGraphPanel>();
            m_HierarchyPanel = m_PanelManager.AddPanel<HierarchyPanel>();
            m_ModelLoaderPanel = m_PanelManager.AddPanel<ModelLoaderPanel>();
            m_TimelinePanel = m_PanelManager.AddPanel<EditorTimelinePanel>();
            m_SceneSettingsPanel = m_PanelManager.AddPanel<SceneSettingsPanel>();
            m_LoadingPanel = m_PanelManager.AddPanel<LoadingPanel>();
            // m_PanelManager.AddPanel<ConsolePanel>();
            // m_PanelManager.AddPanel<AssetsPanel>();
            m_PanelsInitialized = true;

            // Re-apply externally provided pointers now that panels exist.
            if (m_Scene)
                SetScene(m_Scene);
            if (m_AssetLoader)
                SetAssetLoader(m_AssetLoader);
            if (m_ScenePanel && renderer)
                m_ScenePanel->SetRenderer(renderer);
        }
        else if (framebuffer != m_Framebuffer && m_ScenePanel)
        {
            // Update framebuffer if it changed
            m_Framebuffer = framebuffer;
            m_ScenePanel->SetFramebuffer(m_Framebuffer);
        }

        if (m_ScenePanel && renderer)
            m_ScenePanel->SetRenderer(renderer);

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

                // --- Split left for hierarchy ---
                ImGuiID dock_left = ImGui::DockBuilderSplitNode(
                    dockspaceID, ImGuiDir_Left, 0.20f, nullptr, &dockspaceID);

                // --- Split right for inspector ---
                ImGuiID dock_right = ImGui::DockBuilderSplitNode(
                    dockspaceID, ImGuiDir_Right, 0.25f, nullptr, &dockspaceID);

                // --- Split bottom for timeline / console / assets ---
                ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(
                    dockspaceID, ImGuiDir_Down, 0.25f, nullptr, &dockspaceID);

                // Remaining center becomes the scene
                ImGuiID dock_scene = dockspaceID;

                // --- Dock windows ---
                ImGui::DockBuilderDockWindow("Hierarchy", dock_left);
                ImGui::DockBuilderDockWindow("Scene", dock_scene);
                ImGui::DockBuilderDockWindow("Animation Graph", dock_scene);
                ImGui::DockBuilderDockWindow("Timeline", dock_scene);
                ImGui::DockBuilderDockWindow("Inspector", dock_right);

                ImGui::DockBuilderDockWindow("Console", dock_bottom);
                ImGui::DockBuilderDockWindow("Assets", dock_bottom);
                ImGui::DockBuilderDockWindow("Model Loader", dock_bottom);
                ImGui::DockBuilderDockWindow("Scene Settings", dock_bottom);

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
        if (m_ScenePanel)
        {
            m_ScenePanel->SetScene(scene);
            m_ScenePanel->SetHierarchyPanel(m_HierarchyPanel);
        }
        if (m_AnimationGraphPanel)
        {
            m_AnimationGraphPanel->SetScene(scene);
            m_AnimationGraphPanel->SetHierarchyPanel(m_HierarchyPanel);
        }
        if (m_TimelinePanel)
        {
            m_TimelinePanel->SetScene(scene);
            m_TimelinePanel->SetHierarchyPanel(m_HierarchyPanel);
        }
        if (m_SceneSettingsPanel)
            m_SceneSettingsPanel->SetScene(scene);
    }

    void ImGuiLayer::SetAssetLoader(AssetLoader *loader)
    {
        m_AssetLoader = loader;

        // Only apply to panels if they've been initialized
        if (!m_PanelsInitialized)
            return;

        if (m_LoadingPanel)
            m_LoadingPanel->SetAssetLoader(loader);
        if (m_SceneSettingsPanel)
            m_SceneSettingsPanel->SetAssetLoader(loader);
    }

    ImFont *ImGuiLayer::LoadIconFont(const std::string &fontPath, float fontSize, const char *fontName)
    {
        ImGuiIO &io = ImGui::GetIO();

        ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
        if (font)
        {
            m_IconFonts[fontName] = font;
            return font;
        }

        return nullptr;
    }

    ImFont *ImGuiLayer::GetIconFont(const char *fontName) const
    {
        auto it = m_IconFonts.find(fontName);
        if (it != m_IconFonts.end())
            return it->second;
        return nullptr;
    }

    bool ImGuiLayer::HasIconFont(const char *fontName) const
    {
        return m_IconFonts.find(fontName) != m_IconFonts.end();
    }

    void ImGuiLayer::LoadDefaultIconFonts()
    {
        const char *faPathOptions[] = {
            "resources/fonts/FontAwesome/Font Awesome 6 Free-Solid-900.otf",
            "resources/fonts/FontAwesome6-Solid.otf",
            "resources/fonts/FontAwesome/FontAwesome6-Solid.otf",
            "resources/fonts/icons/FontAwesome6-Solid.otf",
            "resources/fonts/FontAwesome/Font Awesome 6 Free-Regular-400.otf",
            "resources/fonts/FontAwesome/Font Awesome 6 Brands-Regular-400.otf"};

        for (const auto &path : faPathOptions)
        {
            ImFont *faFont = LoadIconFont(path, 14.0f, "FontAwesome");
            if (faFont)
            {
                std::cout << "[ImGuiLayer] Loaded fallback FontAwesome icon font from: " << path << std::endl;
                return;
            }
        }

        std::cout << "[ImGuiLayer] No icon font found. To add icon support:" << std::endl;
        std::cout << "  1. Download FontAwesome 6 Free from fontawesome.com/download" << std::endl;
        std::cout << "  2. Extract the downloaded file" << std::endl;
        std::cout << "  3. Copy 'Font Awesome 6 Free-Solid-900.otf' (or FontAwesome6-Solid.otf/.ttf) to resources/fonts/" << std::endl;
        std::cout << "  4. Restart the application" << std::endl;
        std::cout << "  5. Use Icons::* constants from IconDefs.h with GetIconFont()" << std::endl;
    }
}