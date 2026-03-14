#include "SceneSettingsPanel.h"
#include "Scene.h"
#include "AssetLoader.h"
#include "AnimationComponent.h"
#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <glm/glm.hpp>

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

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Animation Playback");
        ImGui::Separator();

        if (ImGui::Checkbox("Play Animations##global", &m_AnimationPlaying))
        {
            // Update all entities with animation components
            for (auto &entity : m_Scene->GetEntities())
            {
                auto *animComp = entity->GetComponent<AnimationComponent>();
                if (animComp)
                {
                    animComp->playing = m_AnimationPlaying;
                }
            }
        }

        if (ImGui::SliderFloat("Playback Speed##global", &m_PlaybackSpeed, 0.0f, 3.0f))
        {
            // Update all entities with animation components
            for (auto &entity : m_Scene->GetEntities())
            {
                auto *animComp = entity->GetComponent<AnimationComponent>();
                if (animComp)
                {
                    animComp->playbackSpeed = m_PlaybackSpeed;
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Lighting");

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

        bool lightGizmoVisible = m_Scene->IsLightGizmoVisible();
        if (ImGui::Checkbox("Show Light Gizmos", &lightGizmoVisible))
        {
            m_Scene->SetLightGizmoVisible(lightGizmoVisible);
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
                // Use async loading if AssetLoader is available
                if (m_AssetLoader)
                {
                    m_AssetLoader->LoadSkyboxAsync(m_Scene, path,
                                                   [this](bool success, const std::string &errorMsg)
                                                   {
                                                       if (success)
                                                       {
                                                           m_Scene->SetSkyboxEnabled(true);
                                                           std::cout << "[SceneSettingsPanel] Skybox loaded successfully!" << std::endl;
                                                       }
                                                       else
                                                       {
                                                           std::cerr << "[SceneSettingsPanel] Failed to load skybox: " << errorMsg << std::endl;
                                                       }
                                                   });
                }
                else
                {
                    // Fallback to synchronous loading
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
        ImGui::Text("Camera");
        ImGui::Separator();

        auto camera = m_Scene->GetCameraSettings();
        if (ImGui::DragFloat3("Position##camera", &camera.Position.x, 0.05f))
        {
            m_Scene->SetCameraSettings(camera);
        }

        float yawDeg = glm::degrees(camera.Yaw);
        if (ImGui::DragFloat("Yaw##camera", &yawDeg, 0.5f))
        {
            camera.Yaw = glm::radians(yawDeg);
            m_Scene->SetCameraSettings(camera);
        }

        float pitchDeg = glm::degrees(camera.Pitch);
        if (ImGui::DragFloat("Pitch##camera", &pitchDeg, 0.5f, -85.0f, 85.0f))
        {
            pitchDeg = glm::clamp(pitchDeg, -85.0f, 85.0f);
            camera.Pitch = glm::radians(pitchDeg);
            m_Scene->SetCameraSettings(camera);
        }

        if (ImGui::SliderFloat("Move Speed##camera", &camera.MoveSpeed, 0.5f, 30.0f))
        {
            m_Scene->SetCameraSettings(camera);
        }

        if (ImGui::SliderFloat("Mouse Sensitivity##camera", &camera.MouseSensitivity, 0.001f, 0.08f, "%.4f"))
        {
            m_Scene->SetCameraSettings(camera);
        }

        if (ImGui::SliderFloat("FOV##camera", &camera.FOV, 20.0f, 100.0f, "%.1f deg"))
        {
            m_Scene->SetCameraSettings(camera);
        }

        if (ImGui::DragFloat("Near Clip##camera", &camera.NearClip, 0.005f, 0.001f, 10.0f, "%.3f"))
        {
            if (camera.NearClip >= camera.FarClip)
                camera.NearClip = glm::max(0.001f, camera.FarClip - 0.001f);
            m_Scene->SetCameraSettings(camera);
        }

        if (ImGui::DragFloat("Far Clip##camera", &camera.FarClip, 0.1f, 1.0f, 5000.0f, "%.1f"))
        {
            if (camera.FarClip <= camera.NearClip)
                camera.FarClip = camera.NearClip + 0.001f;
            m_Scene->SetCameraSettings(camera);
        }

        if (ImGui::Button("Reset Camera##camera"))
        {
            Scene::CameraSettings defaults;
            m_Scene->SetCameraSettings(defaults);
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
                // Use async loading if AssetLoader is available, otherwise use sync
                if (m_AssetLoader)
                {
                    m_AssetLoader->LoadSceneAsync(m_Scene, path,
                                                  [this](bool success, const std::string &errorMsg)
                                                  {
                                                      if (success)
                                                      {
                                                          std::cout << "[SceneSettingsPanel] Scene loaded successfully!" << std::endl;
                                                      }
                                                      else
                                                      {
                                                          std::cerr << "[SceneSettingsPanel] Failed to load scene: " << errorMsg << std::endl;
                                                      }
                                                  });
                }
                else
                {
                    // Fallback to synchronous loading
                    if (m_Scene->LoadFromFile(path))
                    {
                        ImGui::OpenPopup("SceneLoaded");
                    }
                    else
                    {
                        ImGui::OpenPopup("SceneLoadFailed");
                    }
                }
            }
            else
            {
                ImGui::OpenPopup("SceneFileNotFound");
            }
        }

        ImGui::SameLine();

        // File picker for scene loading
        static bool showSceneBrowser = false;
        static std::string sceneCurrentDirectory = std::filesystem::current_path().string();
        static std::vector<std::pair<std::string, bool>> sceneDirectoryContents;
        static std::string sceneSelectedPath;

        if (ImGui::Button("Browse Scene##scene"))
        {
            showSceneBrowser = true;
            try
            {
                sceneDirectoryContents.clear();
                for (const auto &entry : std::filesystem::directory_iterator(sceneCurrentDirectory))
                {
                    sceneDirectoryContents.push_back({entry.path().filename().string(), entry.is_directory()});
                }
                std::sort(sceneDirectoryContents.begin(), sceneDirectoryContents.end(),
                          [](const auto &a, const auto &b)
                          {
                              if (a.second != b.second)
                                  return a.second > b.second;
                              return a.first < b.first;
                          });
            }
            catch (const std::exception &)
            {
            }
        }

        if (showSceneBrowser)
        {
            ImGui::OpenPopup("##SceneFileBrowser");
        }

        if (ImGui::BeginPopupModal("##SceneFileBrowser", &showSceneBrowser, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Current: %s", sceneCurrentDirectory.c_str());
            ImGui::Separator();

            if (ImGui::Button(".. (Parent)##scene_parent"))
            {
                auto parent = std::filesystem::path(sceneCurrentDirectory).parent_path();
                if (!parent.empty() && parent.string() != sceneCurrentDirectory)
                {
                    sceneCurrentDirectory = parent.string();
                    try
                    {
                        sceneDirectoryContents.clear();
                        for (const auto &entry : std::filesystem::directory_iterator(sceneCurrentDirectory))
                        {
                            sceneDirectoryContents.push_back({entry.path().filename().string(), entry.is_directory()});
                        }
                        std::sort(sceneDirectoryContents.begin(), sceneDirectoryContents.end(),
                                  [](const auto &a, const auto &b)
                                  {
                                      if (a.second != b.second)
                                          return a.second > b.second;
                                      return a.first < b.first;
                                  });
                    }
                    catch (const std::exception &)
                    {
                    }
                }
            }

            ImGui::Separator();
            if (ImGui::BeginListBox("##scene_filelist", ImVec2(520, 320)))
            {
                for (const auto &[name, isDir] : sceneDirectoryContents)
                {
                    std::string label = isDir ? "[DIR] " + name : name;
                    bool isSelected = (!isDir && sceneSelectedPath == (std::filesystem::path(sceneCurrentDirectory) / name).string());
                    if (ImGui::Selectable(label.c_str(), isSelected))
                    {
                        if (isDir)
                        {
                            sceneCurrentDirectory = (std::filesystem::path(sceneCurrentDirectory) / name).string();
                            try
                            {
                                sceneDirectoryContents.clear();
                                for (const auto &entry : std::filesystem::directory_iterator(sceneCurrentDirectory))
                                {
                                    sceneDirectoryContents.push_back({entry.path().filename().string(), entry.is_directory()});
                                }
                                std::sort(sceneDirectoryContents.begin(), sceneDirectoryContents.end(),
                                          [](const auto &a, const auto &b)
                                          {
                                              if (a.second != b.second)
                                                  return a.second > b.second;
                                              return a.first < b.first;
                                          });
                            }
                            catch (const std::exception &)
                            {
                            }
                        }
                        else
                        {
                            sceneSelectedPath = (std::filesystem::path(sceneCurrentDirectory) / name).string();
                        }
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::Separator();
            if (!sceneSelectedPath.empty())
            {
                ImGui::TextWrapped("Selected: %s", sceneSelectedPath.c_str());
            }

            if (ImGui::Button("Load Selected##scene_load", ImVec2(160, 0)))
            {
                if (!sceneSelectedPath.empty() && std::filesystem::exists(sceneSelectedPath))
                {
                    // Use async loading if AssetLoader is available
                    if (m_AssetLoader)
                    {
                        m_AssetLoader->LoadSceneAsync(m_Scene, sceneSelectedPath,
                                                      [this](bool success, const std::string &errorMsg)
                                                      {
                                                          if (success)
                                                          {
                                                              std::cout << "[SceneSettingsPanel] Scene loaded successfully!" << std::endl;
                                                          }
                                                          else
                                                          {
                                                              std::cerr << "[SceneSettingsPanel] Failed to load scene: " << errorMsg << std::endl;
                                                          }
                                                      });
                    }
                    else
                    {
                        // Fallback to synchronous loading
                        if (m_Scene->LoadFromFile(sceneSelectedPath))
                        {
                            ImGui::OpenPopup("SceneLoaded");
                        }
                        else
                        {
                            ImGui::OpenPopup("SceneLoadFailed");
                        }
                    }
                    showSceneBrowser = false;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel##scene_cancel", ImVec2(120, 0)))
            {
                showSceneBrowser = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
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
