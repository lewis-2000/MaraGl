#include "ModelLoaderPanel.h"
#include <algorithm>
#include <iostream>

namespace MaraGl
{
    ModelLoaderPanel::ModelLoaderPanel()
    {
        m_CurrentDirectory = std::filesystem::current_path().string();
    }

    void ModelLoaderPanel::OnImGuiRender()
    {
        ImGui::Begin(GetName(), &m_Open);

        ImGui::Text("Model File Path:");
        ImGui::InputText("##ModelPath", m_PathBuffer, sizeof(m_PathBuffer));

        ImGui::SameLine();
        if (ImGui::Button("Browse"))
        {
            m_ShowBrowser = true;
            // Populate directory contents
            try
            {
                m_DirectoryContents.clear();
                for (const auto &entry : std::filesystem::directory_iterator(m_CurrentDirectory))
                {
                    std::string name = entry.path().filename().string();
                    bool isDir = entry.is_directory();
                    m_DirectoryContents.push_back({name, isDir});
                }
                // Sort: directories first, then alphabetically
                std::sort(m_DirectoryContents.begin(), m_DirectoryContents.end(),
                          [](const auto &a, const auto &b)
                          {
                              if (a.second != b.second)
                                  return a.second > b.second; // directories first
                              return a.first < b.first;       // alphabetical
                          });
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error reading directory: " << e.what() << std::endl;
            }
        }

        if (ImGui::Button("Load Model"))
        {
            m_SelectedPath = m_PathBuffer;
            m_ShouldLoadModel = true;
        }

        ImGui::Separator();

        // File browser modal
        if (m_ShowBrowser)
        {
            ImGui::OpenPopup("##BrowseModels");
        }

        if (ImGui::BeginPopupModal("##BrowseModels", &m_ShowBrowser, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Current: %s", m_CurrentDirectory.c_str());

            ImGui::Separator();

            // Parent directory button
            if (ImGui::Button(".. (Parent)##parent"))
            {
                auto parent = std::filesystem::path(m_CurrentDirectory).parent_path();
                if (parent != m_CurrentDirectory)
                {
                    m_CurrentDirectory = parent.string();
                    // Repopulate
                    try
                    {
                        m_DirectoryContents.clear();
                        for (const auto &entry : std::filesystem::directory_iterator(m_CurrentDirectory))
                        {
                            std::string name = entry.path().filename().string();
                            bool isDir = entry.is_directory();
                            m_DirectoryContents.push_back({name, isDir});
                        }
                        std::sort(m_DirectoryContents.begin(), m_DirectoryContents.end(),
                                  [](const auto &a, const auto &b)
                                  {
                                      if (a.second != b.second)
                                          return a.second > b.second;
                                      return a.first < b.first;
                                  });
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error reading directory: " << e.what() << std::endl;
                    }
                }
            }

            ImGui::Separator();

            // File list
            if (ImGui::BeginListBox("##FileList", ImVec2(500, 300)))
            {
                for (const auto &[name, isDir] : m_DirectoryContents)
                {
                    std::string label = isDir ? "[DIR] " + name : name;
                    if (ImGui::Selectable(label.c_str()))
                    {
                        if (isDir)
                        {
                            // Navigate into directory
                            m_CurrentDirectory = (std::filesystem::path(m_CurrentDirectory) / name).string();
                            // Repopulate
                            try
                            {
                                m_DirectoryContents.clear();
                                for (const auto &entry : std::filesystem::directory_iterator(m_CurrentDirectory))
                                {
                                    std::string entryName = entry.path().filename().string();
                                    bool entryIsDir = entry.is_directory();
                                    m_DirectoryContents.push_back({entryName, entryIsDir});
                                }
                                std::sort(m_DirectoryContents.begin(), m_DirectoryContents.end(),
                                          [](const auto &a, const auto &b)
                                          {
                                              if (a.second != b.second)
                                                  return a.second > b.second;
                                              return a.first < b.first;
                                          });
                            }
                            catch (const std::exception &e)
                            {
                                std::cerr << "Error reading directory: " << e.what() << std::endl;
                            }
                        }
                        else
                        {
                            // Select file
                            m_SelectedPath = (std::filesystem::path(m_CurrentDirectory) / name).string();
                            strncpy(m_PathBuffer, m_SelectedPath.c_str(), sizeof(m_PathBuffer) - 1);
                        }
                    }
                }
                ImGui::EndListBox();
            }

            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                m_ShowBrowser = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_ShowBrowser = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }
}
