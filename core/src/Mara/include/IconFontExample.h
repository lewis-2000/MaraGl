/**
 * @file IconFontExample.h
 * @brief Examples of using icon fonts in the editor UI
 *
 * This file demonstrates common patterns for using FontAwesome icons
 * in ImGui panels and UI code. Copy and adapt these patterns for your needs.
 */

#pragma once

#include "IconDefs.h"
#include "ImGuiLayer.h"
#include <imgui.h>

namespace MaraGl::Examples
{
    // Get the icon font from ImGuiLayer (you'll have access to ImGuiLayer in your context)
    // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");

    // ============================================================
    // EXAMPLE 1: Simple button with icon and text
    // ============================================================
    inline void ExampleButtonWithIcon()
    {
        // When you have access to ImGuiLayer:
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        //
        // if (ImGui::Button(Icons::Icon(Icons::FileSave, "Save").c_str()))
        // {
        //     // Handle save action
        // }
    }

    // ============================================================
    // EXAMPLE 2: Icon-only button (compact)
    // ============================================================
    inline void ExampleIconOnlyButton()
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        // if (ImGui::Button(Icons::Play)) { /* play action */ }
        // ImGui::SameLine();
        // if (ImGui::Button(Icons::Pause)) { /* pause action */ }
        // ImGui::SameLine();
        // if (ImGui::Button(Icons::Stop)) { /* stop action */ }
    }

    // ============================================================
    // EXAMPLE 3: Toolbar with mixed icon and text buttons
    // ============================================================
    inline void ExampleToolbar()
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        //
        // if (ImGui::Button(Icons::Icon(Icons::FileNew, "New").c_str()))
        //     { /* new scene */ }
        // ImGui::SameLine();
        //
        // if (ImGui::Button(Icons::Icon(Icons::FileOpen, "Open").c_str()))
        //     { /* open scene */ }
        // ImGui::SameLine();
        //
        // if (ImGui::Button(Icons::Icon(Icons::FileSave, "Save").c_str()))
        //     { /* save scene */ }
        // ImGui::SameLine();
        //
        // ImGui::Separator();
        // ImGui::SameLine();
        //
        // if (ImGui::Button(Icons::Icon(Icons::Undo, "").c_str()))
        //     { /* undo */ }
        // ImGui::SameLine();
        //
        // if (ImGui::Button(Icons::Icon(Icons::Redo, "").c_str()))
        //     { /* redo */ }
    }

    // ============================================================
    // EXAMPLE 4: Tree/Hierarchy with icons
    // ============================================================
    inline void ExampleHierarchyTree()
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        // if (iconFont) ImGui::PushFont(iconFont);
        //
        // if (ImGui::TreeNode(Icons::Icon(Icons::Folder, "Assets").c_str()))
        // {
        //     if (ImGui::TreeNode(Icons::Icon(Icons::Cube, "Models").c_str()))
        //     {
        //         ImGui::Text("%s model1.fbx", Icons::Cube);
        //         ImGui::Text("%s model2.fbx", Icons::Cube);
        //         ImGui::TreePop();
        //     }
        //     ImGui::TreePop();
        // }
        //
        // if (iconFont) ImGui::PopFont();
    }

    // ============================================================
    // EXAMPLE 5: Inspector panel with icons
    // ============================================================
    inline void ExampleInspectorPanel()
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        //
        // ImGui::Text("%s Transform Component", Icons::Move);
        // ImGui::Separator();
        // // ... transform controls
        //
        // ImGui::Spacing();
        // ImGui::Text("%s Render Component", Icons::Eye);
        // ImGui::Separator();
        // // ... render controls
        //
        // ImGui::Spacing();
        // ImGui::Text("%s Light Component", Icons::Lightbulb);
        // ImGui::Separator();
        // // ... light controls
    }

    // ============================================================
    // EXAMPLE 6: Status indicators with icons
    // ============================================================
    inline void ExampleStatusIndicators()
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        //
        // ImGui::TextColored(ImVec4(0,1,0,1), "%s Scene Loaded", Icons::Check);
        // ImGui::TextColored(ImVec4(1,1,0,1), "%s Unsaved Changes", Icons::Warning);
        // ImGui::TextColored(ImVec4(1,0,0,1), "%s Error Loading File", Icons::Error);
    }

    // ============================================================
    // EXAMPLE 7: Context menu with icons
    // ============================================================
    inline void ExampleContextMenu()
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        //
        // if (ImGui::BeginPopupContextItem())
        // {
        //     if (ImGui::MenuItem(Icons::Icon(Icons::Edit, "Rename").c_str()))
        //         { /* rename */ }
        //     if (ImGui::MenuItem(Icons::Icon(Icons::Copy, "Duplicate").c_str()))
        //         { /* duplicate */ }
        //     ImGui::Separator();
        //     if (ImGui::MenuItem(Icons::Icon(Icons::Delete, "Delete").c_str()))
        //         { /* delete */ }
        //     ImGui::EndPopup();
        // }
    }

    // ============================================================
    // EXAMPLE 8: Animation controls with icons and text
    // ============================================================
    inline void ExampleAnimationControls()
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        //
        // ImGui::AlignTextToFramePadding();
        // ImGui::Text("%s Animation:", Icons::Film);
        // ImGui::SameLine();
        //
        // if (ImGui::Button(Icons::StepBack)) { /* step back */ }
        // ImGui::SameLine();
        // if (ImGui::Button(Icons::Play)) { /* play */ }
        // ImGui::SameLine();
        // if (ImGui::Button(Icons::Pause)) { /* pause */ }
        // ImGui::SameLine();
        // if (ImGui::Button(Icons::Stop)) { /* stop */ }
        // ImGui::SameLine();
        // if (ImGui::Button(Icons::StepForward)) { /* step forward */ }
    }

    // ============================================================
    // EXAMPLE 9: Toggle buttons with icons
    // ============================================================
    inline void ExampleToggleButtons(bool &visible, bool &locked)
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        //
        // // Visibility toggle
        // if (ImGui::Button(visible ? Icons::Eye : Icons::EyeSlash))
        //     visible = !visible;
        // ImGui::SameLine();
        //
        // // Lock toggle
        // if (ImGui::Button(locked ? Icons::Lock : Icons::Unlock))
        //     locked = !locked;
    }

    // ============================================================
    // EXAMPLE 10: Property panel with component icons
    // ============================================================
    inline void ExamplePropertyPanel()
    {
        // ImFont* iconFont = imGuiLayer->GetIconFont("FontAwesome");
        //
        // // File operations section
        // if (ImGui::CollapsingHeader(Icons::Icon(Icons::Folder, "File").c_str()))
        // {
        //     if (ImGui::Button(Icons::Icon(Icons::FileOpen, "Open").c_str(), ImVec2(-1, 0)))
        //         { /* open */ }
        //     if (ImGui::Button(Icons::Icon(Icons::FileSave, "Save").c_str(), ImVec2(-1, 0)))
        //         { /* save */ }
        // }
        //
        // // Edit operations section
        // if (ImGui::CollapsingHeader(Icons::Icon(Icons::Edit, "Edit").c_str()))
        // {
        //     if (ImGui::Button(Icons::Icon(Icons::Undo, "Undo").c_str(), ImVec2(-1, 0)))
        //         { /* undo */ }
        //     if (ImGui::Button(Icons::Icon(Icons::Redo, "Redo").c_str(), ImVec2(-1, 0)))
        //         { /* redo */ }
        // }
    }

    // ============================================================
    // HELPER: Push/Pop icon font safely
    // ============================================================
    class IconFontScoped
    {
    public:
        IconFontScoped(ImFont *font)
        {
            if (font)
                ImGui::PushFont(font);
            m_Font = font;
        }

        ~IconFontScoped()
        {
            if (m_Font)
                ImGui::PopFont();
        }

    private:
        ImFont *m_Font = nullptr;
    };

    // Usage: IconFontScoped iconFont(imGuiLayer->GetIconFont("FontAwesome"));
    // All ImGui calls within scope will use the icon font
}

/*
 * INTEGRATION GUIDE
 *
 * To use these examples in your panels:
 *
 * 1. Add to your panel header:
 *    #include "IconDefs.h"
 *
 * 2. In your panel class, get the icon font from ImGuiLayer:
 *    ImFont* iconFont = m_Scene->GetImGuiLayer()->GetIconFont("FontAwesome");
 *
 * 3. In your OnImGuiRender() method:
 *    - Use Icons::* constants
 *    - Push the icon font when you need icons
 *    - Pop the font to return to normal text
 *
 * Example in a panel:
 *
 *    void MyPanel::OnImGuiRender()
 *    {
 *        ImGui::Begin("My Panel");
 *
 *        ImFont* iconFont = m_ImGuiLayer->GetIconFont("FontAwesome");
 *        if (iconFont) ImGui::PushFont(iconFont);
 *
 *        if (ImGui::Button(Icons::Icon(Icons::FileSave, "Save").c_str()))
 *        {
 *            // Handle save
 *        }
 *
 *        if (iconFont) ImGui::PopFont();
 *
 *        ImGui::End();
 *    }
 */
