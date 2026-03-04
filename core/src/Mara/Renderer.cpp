#include "Renderer.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace MaraGl
{
    Renderer::Renderer()
    {
        // Shader
        m_Shader = new Shader(
            "resources/shaders/basic.vert",
            "resources/shaders/basic.frag");

        // Use EditorCamera for input-responsive viewport
        m_Camera = new EditorCamera(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f);
        m_Shader->use();
        m_Shader->setMat4("view", m_Camera->GetView());
        m_Shader->setMat4("projection", m_Camera->GetProjection());
        m_Shader->use();
        m_Shader->setMat4("view", m_Camera->GetView());
        m_Shader->setMat4("projection", m_Camera->GetProjection());
    }

    Renderer::~Renderer()
    {
        delete m_Shader;
        delete m_Camera;
    }

    void Renderer::clear(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::drawToFramebuffer(Framebuffer &framebuffer)
    {
        framebuffer.bind();
        glEnable(GL_DEPTH_TEST);

        clear(0.1f, 0.2f, 0.3f, 1.0f);
        framebuffer.unbind();
    }

    void Renderer::DrawModel(Model &model, Shader &shader)
    {
        shader.use();

        glm::mat4 modelMat(1.0f);
        modelMat = glm::scale(modelMat, glm::vec3(m_Settings.modelScale));

        shader.setMat4("model", modelMat);
        shader.setMat4("view", m_Camera->GetView());
        shader.setMat4("projection", m_Camera->GetProjection());

        shader.setVec3("uViewPos", m_Camera->GetPosition());
        shader.setVec3("uLightDir", glm::normalize(m_Settings.lightDir));
        shader.setVec3("uLightColor", m_Settings.lightColor);
        shader.setVec3("uObjectColor", m_Settings.objectColor);

        shader.setBool("uUseTexture", m_Settings.useTexture);
        shader.setInt("uDiffuseMap", 0);

        shader.setFloat("uAmbientStrength", m_Settings.ambientStrength);
        shader.setFloat("uSpecularStrength", m_Settings.specularStrength);
        shader.setFloat("uShininess", m_Settings.shininess);

        model.Draw(shader);
    }
}