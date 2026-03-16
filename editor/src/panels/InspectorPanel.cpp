#include "InspectorPanel.h"
#include "Scene.h"
#include "HierarchyPanel.h"
#include "NameComponent.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "LightComponent.h"
#include "AnimationComponent.h"
#include "Animator.h"
#include "Model.h"
#include "IconDefs.h"
#include <memory>
#include <filesystem>
#include <algorithm>
#include <vector>

namespace MaraGl
{
    void InspectorPanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        if (!m_Scene || !m_HierarchyPanel)
        {
            ImGui::Text("Scene or HierarchyPanel not set");
            ImGui::End();
            return;
        }

        uint32_t selectedID = m_HierarchyPanel->GetSelectedEntityID();
        if (selectedID == 0)
        {
            ImGui::Text("No entity selected");
            ImGui::End();
            return;
        }

        Entity *entity = m_Scene->FindEntityByID(selectedID);
        if (!entity)
        {
            m_HierarchyPanel->SetSelectedEntityID(0);
            ImGui::Text("Selected entity no longer exists");
            ImGui::End();
            return;
        }

        ImGui::Text("Entity ID: %u", selectedID);
        ImGui::Separator();

        // Display and edit components
        auto components = m_Scene->GetRegistry().GetEntityComponents(selectedID);
        bool removeNameComponent = false;
        bool removeTransformComponent = false;
        bool removeMeshComponent = false;
        bool removeLightComponent = false;
        bool removeAnimationComponent = false;

        ImGui::Text("Components: %zu", components.size());
        ImGui::Separator();

        for (auto comp : components)
        {
            if (!comp)
                continue;

            // Get component type name
            const char *typeName = typeid(*comp).name();

            // Clean up the RTTI name (remove "struct MaraGl::" prefix if present)
            std::string cleanName = typeName;
            size_t pos = cleanName.find_last_of(':');
            if (pos != std::string::npos)
                cleanName = cleanName.substr(pos + 1);

            ImGui::PushID(comp);
            bool open = ImGui::CollapsingHeader(cleanName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            ImGui::SameLine();
            if (dynamic_cast<NameComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##name"))
                    removeNameComponent = true;
            }
            else if (dynamic_cast<TransformComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##transform"))
                    removeTransformComponent = true;
            }
            else if (dynamic_cast<MeshComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##mesh"))
                    removeMeshComponent = true;
            }
            else if (dynamic_cast<LightComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##light"))
                    removeLightComponent = true;
            }
            else if (dynamic_cast<AnimationComponent *>(comp))
            {
                if (ImGui::SmallButton("Remove##animation"))
                    removeAnimationComponent = true;
            }

            if (open)
            {
                comp->OnImGuiRender();

                // Special handling for MeshComponent - add button to load model
                if (auto *meshComp = dynamic_cast<MeshComponent *>(comp))
                {
                    ImGui::Spacing();

                    // File picker state (shared UI state for inspector model selection)
                    static bool showModelBrowser = false;
                    static std::string currentDirectory = std::filesystem::current_path().string();
                    static std::vector<std::pair<std::string, bool>> directoryContents;
                    static std::string selectedPath;

                    if (ImGui::Button(Icons::Icon(Icons::Search, "Browse Model##mesh").c_str()))
                    {
                        showModelBrowser = true;
                        try
                        {
                            directoryContents.clear();
                            for (const auto &entry : std::filesystem::directory_iterator(currentDirectory))
                            {
                                directoryContents.push_back({entry.path().filename().string(), entry.is_directory()});
                            }
                            std::sort(directoryContents.begin(), directoryContents.end(),
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

                    if (!meshComp->ModelPath.empty())
                    {
                        ImGui::TextWrapped("Model: %s", meshComp->ModelPath.c_str());
                    }

                    if (showModelBrowser)
                    {
                        ImGui::OpenPopup("##InspectorModelBrowser");
                    }

                    if (ImGui::BeginPopupModal("##InspectorModelBrowser", &showModelBrowser, ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        ImGui::Text("Current: %s", currentDirectory.c_str());
                        ImGui::Separator();

                        if (ImGui::Button(Icons::Icon(Icons::ChevronLeft, "Parent##inspector_parent").c_str()))
                        {
                            auto parent = std::filesystem::path(currentDirectory).parent_path();
                            if (!parent.empty() && parent.string() != currentDirectory)
                            {
                                currentDirectory = parent.string();
                                try
                                {
                                    directoryContents.clear();
                                    for (const auto &entry : std::filesystem::directory_iterator(currentDirectory))
                                    {
                                        directoryContents.push_back({entry.path().filename().string(), entry.is_directory()});
                                    }
                                    std::sort(directoryContents.begin(), directoryContents.end(),
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
                        if (ImGui::BeginListBox("##inspector_filelist", ImVec2(520, 320)))
                        {
                            for (const auto &[name, isDir] : directoryContents)
                            {
                                std::string label = isDir ? Icons::Icon(Icons::Folder, name.c_str()) : Icons::Icon(Icons::FileOpen, name.c_str());
                                bool isSelected = (!isDir && selectedPath == (std::filesystem::path(currentDirectory) / name).string());
                                if (ImGui::Selectable(label.c_str(), isSelected))
                                {
                                    if (isDir)
                                    {
                                        currentDirectory = (std::filesystem::path(currentDirectory) / name).string();
                                        try
                                        {
                                            directoryContents.clear();
                                            for (const auto &entry : std::filesystem::directory_iterator(currentDirectory))
                                            {
                                                directoryContents.push_back({entry.path().filename().string(), entry.is_directory()});
                                            }
                                            std::sort(directoryContents.begin(), directoryContents.end(),
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
                                        selectedPath = (std::filesystem::path(currentDirectory) / name).string();
                                    }
                                }
                            }
                            ImGui::EndListBox();
                        }

                        ImGui::Separator();
                        if (!selectedPath.empty())
                        {
                            ImGui::TextWrapped("Selected: %s", selectedPath.c_str());
                        }

                        if (ImGui::Button(Icons::Icon(Icons::Check, "Load Selected##inspector_load").c_str(), ImVec2(160, 0)))
                        {
                            if (!selectedPath.empty() && std::filesystem::exists(selectedPath))
                            {
                                try
                                {
                                    meshComp->ModelPtr = std::make_shared<Model>(selectedPath);
                                    meshComp->ModelPath = selectedPath;
                                    ImGui::OpenPopup("ModelLoadSuccess");
                                    showModelBrowser = false;
                                    ImGui::CloseCurrentPopup();
                                }
                                catch (const std::exception &)
                                {
                                }
                            }
                        }

                        ImGui::SameLine();
                        if (ImGui::Button(Icons::Icon(Icons::Times, "Cancel##inspector_cancel").c_str(), ImVec2(120, 0)))
                        {
                            showModelBrowser = false;
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button(Icons::Icon(Icons::Delete, "Clear Model##mesh").c_str()))
                    {
                        meshComp->ModelPtr.reset();
                    }

                    if (ImGui::BeginPopup("ModelLoadSuccess"))
                    {
                        ImGui::Text("Model loaded successfully!");
                        ImGui::EndPopup();
                    }
                }

                // Special handling for AnimationComponent - add button to load animations from model
                if (auto *animComp = dynamic_cast<AnimationComponent *>(comp))
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("%s Animation Loading", Icons::Film);

                    if (animComp->graphEnabled)
                    {
                        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f),
                                           "Graph mode active: clips are resolved from the animation graph library.");
                        ImGui::TextWrapped("Model clip import is disabled while graph mode is enabled.");
                    }
                    else
                    {
                        // Check if entity has a MeshComponent with a model
                        auto *meshComp = entity->GetComponent<MeshComponent>();
                        if (meshComp && meshComp->ModelPtr && meshComp->ModelPtr->HasAnimations())
                        {
                            ImGui::Text("Model has %d animation(s)", meshComp->ModelPtr->GetAnimationCount());

                            if (ImGui::Button(Icons::Icon(Icons::FileImport, "Load Animations from Model").c_str()))
                            {
                                // Load animations from the model
                                animComp->animations = meshComp->ModelPtr->LoadAnimations();
                                animComp->boneInfoMap = meshComp->ModelPtr->GetBoneInfoMap();

                                // Initialize bone transforms array with identity matrices
                                int boneCount = meshComp->ModelPtr->GetBoneCount();
                                animComp->boneTransforms.clear();
                                animComp->boneTransforms.resize(boneCount, glm::mat4(1.0f));

                                // Calculate initial bind pose transforms
                                if (!animComp->animations.empty())
                                {
                                    animComp->currentAnimation = 0;
                                    animComp->currentTime = 0.0f;

                                    // Calculate bone transforms for first frame
                                    const aiScene *scene = meshComp->ModelPtr->GetScene();
                                    if (scene && scene->mRootNode)
                                    {
                                        glm::mat4 identity(1.0f);
                                        glm::mat4 globalInverseTransform = Animator::GetGlobalInverseTransform(scene);
                                        Animator::CalculateBoneTransform(animComp, animComp->animations[0],
                                                                         scene->mRootNode, identity, globalInverseTransform);
                                    }
                                }

                                ImGui::OpenPopup("AnimationsLoadSuccess");
                            }

                            if (ImGui::BeginPopup("AnimationsLoadSuccess"))
                            {
                                ImGui::Text("Loaded %d animations!", (int)animComp->animations.size());
                                ImGui::EndPopup();
                            }
                        }
                        else if (meshComp && meshComp->ModelPtr)
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Model has no animations");
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No model loaded - add MeshComponent first");
                        }
                    }
                }
            }

            ImGui::PopID();
        }

        if (removeNameComponent)
            entity->RemoveComponent<NameComponent>();
        if (removeTransformComponent)
            entity->RemoveComponent<TransformComponent>();
        if (removeMeshComponent)
            entity->RemoveComponent<MeshComponent>();
        if (removeLightComponent)
            entity->RemoveComponent<LightComponent>();
        if (removeAnimationComponent)
            entity->RemoveComponent<AnimationComponent>();

        ImGui::Separator();

        // Add component dropdown
        if (ImGui::Button(Icons::Icon(Icons::Plus, "Add Component").c_str()))
        {
            ImGui::OpenPopup("AddComponentMenu");
        }

        if (ImGui::BeginPopup("AddComponentMenu"))
        {
            if (ImGui::MenuItem("Name"))
            {
                if (!entity->HasComponent<NameComponent>())
                {
                    auto &nameComp = entity->AddComponent<NameComponent>();
                    nameComp.Name = "New Entity";
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Transform"))
            {
                if (!entity->HasComponent<TransformComponent>())
                {
                    entity->AddComponent<TransformComponent>();
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Mesh"))
            {
                if (!entity->HasComponent<MeshComponent>())
                {
                    entity->AddComponent<MeshComponent>();
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Light"))
            {
                if (!entity->HasComponent<LightComponent>())
                {
                    entity->AddComponent<LightComponent>();
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Animation"))
            {
                if (!entity->HasComponent<AnimationComponent>())
                {
                    entity->AddComponent<AnimationComponent>();
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }
}
