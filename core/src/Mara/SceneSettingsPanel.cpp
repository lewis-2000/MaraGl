#include "SceneSettingsPanel.h"
#include "Scene.h"
#include <imgui.h>
#include <filesystem>

namespace MaraGl
{
    void SceneSettingsPanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        if (!m_Scene)
        {
            ImGui::Text("No scene loaded");
            ImGui::End();
            return;
        }

        ImGui::Separator();
        ImGui::Text("Skybox / Environment");
        ImGui::Separator();

        // Skybox enabled toggle
        bool skyboxEnabled = m_Scene->IsSkyboxEnabled();
        if (ImGui::Checkbox("Enable Skybox", &skyboxEnabled))
        {
            m_Scene->SetSkyboxEnabled(skyboxEnabled);
        }

        ImGui::Spacing();

        // Skybox file path input
        ImGui::InputText("Skybox Path##env", skyboxPathBuffer, sizeof(skyboxPathBuffer));

        ImGui::SameLine();
        if (ImGui::Button("Load HDRI"))
        {
            std::string path(skyboxPathBuffer);
            if (std::filesystem::exists(path))
            {
                if (m_Scene->LoadSkybox(path))
                {
                    ImGui::OpenPopup("SkyboxLoadSuccess");
                    m_Scene->SetSkyboxEnabled(true);
                }
                else
                {
                    ImGui::OpenPopup("SkyboxLoadError");
                }
            }
            else
            {
                ImGui::OpenPopup("FileNotFound");
            }
        }

        // Success popup
        if (ImGui::BeginPopup("SkyboxLoadSuccess"))
        {
            ImGui::Text("Skybox loaded successfully!");
            if (ImGui::Button("Close##success"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Error popup
        if (ImGui::BeginPopup("SkyboxLoadError"))
        {
            ImGui::Text("Failed to load skybox!");
            if (ImGui::Button("Close##error"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // File not found popup
        if (ImGui::BeginPopup("FileNotFound"))
        {
            ImGui::Text("File not found!");
            if (ImGui::Button("Close##notfound"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::End();
    }
}
