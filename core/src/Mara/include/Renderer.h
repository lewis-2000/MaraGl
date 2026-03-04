#pragma once

#include "Shader.h"
#include "Framebuffer.h"
#include "Model.h"
#include "EditorCamera.h"
#include <glm/glm.hpp>

namespace MaraGl
{
    struct RenderSettings
    {
        float modelScale = 0.2f;
        bool useTexture = true;

        glm::vec3 lightDir = glm::vec3(-0.5f, -1.0f, -0.2f);
        glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 objectColor = glm::vec3(0.9f, 0.85f, 0.8f);

        float ambientStrength = 0.30f;
        float specularStrength = 0.25f;
        float shininess = 32.0f;
    };

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        void clear(float r, float g, float b, float a);
        void drawToFramebuffer(Framebuffer &framebuffer);

        void DrawModel(Model &model, Shader &shader);

        // Camera accessor
        EditorCamera &GetCamera() { return *m_Camera; }
        RenderSettings &GetSettings() { return m_Settings; }

    private:
        Shader *m_Shader;
        EditorCamera *m_Camera;
        Model *m_Model;
        RenderSettings m_Settings;
    };
}