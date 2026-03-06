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
        ImGui::Text("Lighting");
        ImGui::Separator();

        // Overcast light toggle and settings
        bool overcastEnabled = m_Scene->IsOvercastEnabled();
        if (ImGui::Checkbox("Enable Overcast Light", &overcastEnabled))
        {
            m_Scene->SetOvercastEnabled(overcastEnabled);
        }

        if (overcastEnabled)
        {
            ImGui::Indent();

            glm::vec3 overcastColor = m_Scene->GetOvercastColor();
            if (ImGui::ColorEdit3("Overcast Color##overcast", &overcastColor.x))
            {
                m_Scene->SetOvercastColor(overcastColor);
            }

            float overcastIntensity = m_Scene->GetOvercastIntensity();
            if (ImGui::SliderFloat("Overcast Intensity##overcast", &overcastIntensity, 0.0f, 2.0f))
            {
                m_Scene->SetOvercastIntensity(overcastIntensity);
            }

            ImGui::Unindent();
        }

        ImGui::Spacing();
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

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Scene Management");
        ImGui::Separator();

        // Show current working directory as a hint
        static std::string currentDir = std::filesystem::current_path().string();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Working Directory:");
        ImGui::SameLine();
        ImGui::TextWrapped("%s", currentDir.c_str());
        ImGui::Spacing();

        // Scene file path input
        ImGui::InputText("Scene File##scene", scenePathBuffer, sizeof(scenePathBuffer));
        ImGui::SameLine();
        ImGui::TextDisabled("(.json)");

        // Save/Load buttons
        if (ImGui::Button("Save Scene##scene"))
        {
            std::string path(scenePathBuffer);
            if (m_Scene->SaveToFile(path))
            {
                ImGui::OpenPopup("SceneSaved");
            }
            else
            {
                ImGui::OpenPopup("SceneSaveFailed");
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Load Scene##scene"))
        {
            std::string path(scenePathBuffer);
            if (std::filesystem::exists(path))
            {
                if (m_Scene->LoadFromFile(path))
                {
                    ImGui::OpenPopup("SceneLoaded");
                }
                else
                {
                    ImGui::OpenPopup("SceneLoadFailed");
                }
            }
            else
            {
                ImGui::OpenPopup("SceneFileNotFound");
            }
        }

        // Scene save success popup
        if (ImGui::BeginPopup("SceneSaved"))
        {
            ImGui::Text("Scene saved successfully!");
            if (ImGui::Button("Close##scenesaved"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Scene save failed popup
        if (ImGui::BeginPopup("SceneSaveFailed"))
        {
            ImGui::Text("Failed to save scene!");
            if (ImGui::Button("Close##scnesavefailed"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Scene load success popup
        if (ImGui::BeginPopup("SceneLoaded"))
        {
            ImGui::Text("Scene loaded successfully!");
            if (ImGui::Button("Close##sceneloaded"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Scene load failed popup
        if (ImGui::BeginPopup("SceneLoadFailed"))
        {
            ImGui::Text("Failed to load scene!");
            if (ImGui::Button("Close##sceneloadfailed"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // Scene file not found popup
        if (ImGui::BeginPopup("SceneFileNotFound"))
        {
            ImGui::Text("Scene file not found!");
            if (ImGui::Button("Close##scenenotfound"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::End();
    }
}
