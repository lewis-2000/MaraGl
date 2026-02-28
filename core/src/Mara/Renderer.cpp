#include "Renderer.h"

#include <glad/glad.h>

namespace MaraGl
{
    Renderer::Renderer()
    {
        // Triangle vertices (position only)
        float vertices[] =
            {
                // positions
                0.0f, 0.5f, 0.0f,
                0.5f, -0.5f, 0.0f,
                -0.5f, -0.5f, 0.0f};

        unsigned int indices[] =
            {
                0, 1, 2};

        m_VAO.Bind();

        m_VBO = new VBO(vertices, sizeof(vertices));
        m_EBO = new EBO(indices, sizeof(indices));

        m_VAO.LinkAttrib(*m_VBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void *)0);

        m_VAO.Unbind();
        m_VBO->Unbind();
        m_EBO->Unbind();

        m_Shader = new Shader(
            "resources/shaders/basic.vert",
            "resources/shaders/basic.frag");
    }

    Renderer::~Renderer()
    {
        delete m_VBO;
        delete m_EBO;
        delete m_Shader;
    }

    void Renderer::drawToFramebuffer(Framebuffer &framebuffer)
    {
        framebuffer.bind();

        glEnable(GL_DEPTH_TEST);

        clear(0.1f, 0.2f, 0.3f, 1.0f);
        draw();

        framebuffer.unbind();
    }

    void Renderer::clear(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Renderer::draw()
    {
        m_Shader->use();
        m_VAO.Bind();
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    }
}