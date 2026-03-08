#pragma once

#include "EditorPanel.h"
#include "AssetLoader.h"
#include <memory>

namespace MaraGl
{
    class LoadingPanel : public EditorPanel
    {
    public:
        LoadingPanel();
        ~LoadingPanel() override = default;

        const char *GetName() const override { return "Asset Loading"; }
        void OnImGuiRender() override;
        void SetAssetLoader(AssetLoader *loader) { m_AssetLoader = loader; }

    private:
        AssetLoader *m_AssetLoader = nullptr;
        bool m_ShowPanel = true;
    };
}
