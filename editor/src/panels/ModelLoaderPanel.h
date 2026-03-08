#pragma once
#include "EditorPanel.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <filesystem>

namespace MaraGl
{
    class ModelLoaderPanel : public EditorPanel
    {
    public:
        ModelLoaderPanel();
        ~ModelLoaderPanel() override = default;

        const char *GetName() const override { return "Model Loader"; }

        void OnImGuiRender() override;

        // Check if a new model should be loaded
        bool ShouldLoadModel() const { return m_ShouldLoadModel; }

        // Get the selected model path and reset the flag
        std::string GetSelectedModelPath()
        {
            m_ShouldLoadModel = false;
            return m_SelectedPath;
        }

    private:
        void BrowseForFile();

    private:
        char m_PathBuffer[512] = "resources/models/Tree/trees9.obj";
        std::string m_SelectedPath;
        bool m_ShouldLoadModel = false;
        bool m_ShowBrowser = false;
        std::string m_CurrentDirectory;
        std::vector<std::pair<std::string, bool>> m_DirectoryContents; // name, isDirectory
    };
}
