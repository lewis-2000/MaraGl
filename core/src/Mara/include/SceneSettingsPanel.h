#pragma once
#include "EditorPanel.h"
#include <string>

namespace MaraGl
{
    class Scene;

    class SceneSettingsPanel : public EditorPanel
    {
    public:
        SceneSettingsPanel() : EditorPanel(), m_Scene(nullptr) {}

        const char *GetName() const override { return "Scene Settings"; }

        void OnImGuiRender() override;

        void SetScene(Scene *scene) { m_Scene = scene; }

    private:
        Scene *m_Scene;
        char skyboxPathBuffer[512] = "resources/skybox.hdr";
        char scenePathBuffer[512] = "scenes/scene.json";
    };
}
