#include "ImGuiLayer.h"

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

    // Initializes ImGui context, backends (GLFW + OpenGL3), and loads fonts
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

        ImGui::StyleColorsDark();

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

    // Renders the editor UI layout with dockspace and all editor panels
    // Layout: Inspector (left 20%) | Scene (center) | Hierarchy (right 20%)
    //         Console + Assets (bottom 25%)
    // \param framebuffer Pointer to the framebuffer to display in the Scene viewport
    void ImGuiLayer::renderDockspace(Framebuffer *framebuffer)
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        // Set dockspace to fill the entire viewport
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        // Configure window flags for dockspace container
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

        ImGui::Begin("EditorDockspace", nullptr, window_flags);

        ImGuiIO &io = ImGui::GetIO();
        ImGuiID dockspaceID = ImGui::GetID("MainDockspace");
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

            // Initialize dock layout only on first frame
            // Uses static bool to ensure this runs once per application lifetime
            static bool firstTime = true;
            if (firstTime)
            {
                firstTime = false;

                // Clear previous layout to rebuild from scratch
                ImGui::DockBuilderRemoveNode(dockspaceID);
                ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->WorkSize);

                // --- Define dock layout ---
                // Split left ~20% for Inspector
                ImGuiID dock_left = ImGui::DockBuilderSplitNode(
                    dockspaceID, ImGuiDir_Left, 0.20f, nullptr, &dockspaceID);

                // Split right ~20% for Hierarchy (from remaining center)
                ImGuiID dock_right = ImGui::DockBuilderSplitNode(
                    dockspaceID, ImGuiDir_Right, 0.20f, nullptr, &dockspaceID);

                // Split bottom ~25% for Console and Assets (from remaining center)
                ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(
                    dockspaceID, ImGuiDir_Down, 0.25f, nullptr, &dockspaceID);

                // Assign windows to their respective dock areas
                ImGui::DockBuilderDockWindow("Inspector", dock_left);
                ImGui::DockBuilderDockWindow("Hierarchy", dock_right);
                ImGui::DockBuilderDockWindow("Scene", dockspaceID); // Central area
                ImGui::DockBuilderDockWindow("Console", dock_bottom);
                ImGui::DockBuilderDockWindow("Assets", dock_bottom);

                ImGui::DockBuilderFinish(dockspaceID);
            }
        }

        // --- Editor Panels ---

        // Inspector Panel (left): Shows properties of selected entities
        ImGui::Begin("Inspector");
        ImGui::Text("Entity properties...");
        ImGui::End();

        // Hierarchy Panel (right): Shows scene tree and entity list
        ImGui::Begin("Hierarchy");
        ImGui::Text("Scene objects...");
        ImGui::End();

        // Scene Viewport Panel (center): Displays rendered scene with framebuffer texture
        ImGui::Begin("Scene");
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        // Only render if framebuffer exists and viewport has non-zero dimensions
        if (framebuffer && viewportSize.x > 0 && viewportSize.y > 0)
        {
            // Automatically resize framebuffer if viewport dimensions changed
            // This keeps the rendered texture in sync with the window size
            if ((int)viewportSize.x != framebuffer->getWidth() ||
                (int)viewportSize.y != framebuffer->getHeight())
            {
                framebuffer->resize((unsigned int)viewportSize.x, (unsigned int)viewportSize.y);
            }

            // Display the framebuffer's color attachment as an image
            // ImVec2(0, 1) to (1, 0) flips Y-axis (OpenGL convention)
            ImGui::Image(
                (ImTextureID)(uintptr_t)framebuffer->getColorAttachment(),
                viewportSize,
                ImVec2(0, 1), ImVec2(1, 0));
        }
        ImGui::End();

        // Console Panel (bottom tab): Displays log output and error messages
        ImGui::Begin("Console");
        ImGui::Text("Console output...");
        ImGui::End();

        // Assets Panel (bottom tab): Browser for project resources and content
        ImGui::Begin("Assets");
        ImGui::Text("Assets browser...");

        {
            // Sample asset thumbnails area
            // TODO: Replace with actual asset database and dynamic thumbnail loading
            ImGui::BeginChild("AssetThumbnails", ImVec2(0, 200), true);
            for (int i = 0; i < 10; i++)
            {
                ImGui::Image((ImTextureID)(uintptr_t)framebuffer->getColorAttachment(), ImVec2(64, 64));
                // Display 5 thumbnails per row
                if ((i + 1) % 5 != 0)
                    ImGui::SameLine();
            }
            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::End(); // End EditorDockspace

        // --- Constraints ---
        // Set minimum size constraint for dockspace containers
        ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(FLT_MAX, FLT_MAX));
    }

}