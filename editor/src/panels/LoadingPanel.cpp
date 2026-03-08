#include "LoadingPanel.h"
#include <imgui.h>
#include <string>

namespace MaraGl
{
    LoadingPanel::LoadingPanel()
    {
    }

    void LoadingPanel::OnImGuiRender()
    {
        if (!m_AssetLoader)
            return;

        // Only show the panel if there are active loads
        if (!m_AssetLoader->IsLoading())
            return;

        // Set window flags for a non-intrusive overlay
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings;

        // Position the window in the bottom-right corner
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImVec2 workPos = viewport->WorkPos;
        ImVec2 workSize = viewport->WorkSize;
        ImVec2 windowPos;
        windowPos.x = workPos.x + workSize.x - 10.0f;
        windowPos.y = workPos.y + workSize.y - 10.0f;
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));

        // Set a semi-transparent background
        ImGui::SetNextWindowBgAlpha(0.85f);

        if (ImGui::Begin(GetName(), nullptr, windowFlags))
        {
            auto loadingItems = m_AssetLoader->GetLoadingProgress();

            if (loadingItems.empty())
            {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Processing...");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Loading Assets...");
                ImGui::Separator();

                for (const auto &item : loadingItems)
                {
                    ImGui::PushID(&item);

                    // Asset type icon
                    const char *typeIcon = "?";
                    ImVec4 typeColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                    switch (item.type)
                    {
                    case AssetType::Model:
                        typeIcon = "M";
                        typeColor = ImVec4(0.8f, 0.5f, 0.8f, 1.0f);
                        break;
                    case AssetType::Scene:
                        typeIcon = "S";
                        typeColor = ImVec4(0.5f, 0.8f, 0.5f, 1.0f);
                        break;
                    case AssetType::Skybox:
                        typeIcon = "K";
                        typeColor = ImVec4(0.5f, 0.7f, 1.0f, 1.0f);
                        break;
                    case AssetType::Texture:
                        typeIcon = "T";
                        typeColor = ImVec4(1.0f, 0.7f, 0.4f, 1.0f);
                        break;
                    }

                    ImGui::TextColored(typeColor, "[%s]", typeIcon);
                    ImGui::SameLine();
                    ImGui::Text("%s", item.assetName.c_str());

                    // Progress bar
                    char progressText[32];
                    snprintf(progressText, sizeof(progressText), "%.0f%%", item.progress * 100.0f);

                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
                    ImGui::ProgressBar(item.progress, ImVec2(-1.0f, 0.0f), progressText);
                    ImGui::PopStyleColor();

                    if (!item.isComplete && item.progress > 0.0f)
                    {
                        ImGui::SameLine();
                        // Animated loading dots
                        int dots = (int)(ImGui::GetTime() * 2.0f) % 4;
                        const char *dotStrs[] = {"", ".", "..", "..."};
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", dotStrs[dots]);
                    }

                    ImGui::PopID();
                    ImGui::Spacing();
                }
            }
        }
        ImGui::End();
    }
}
