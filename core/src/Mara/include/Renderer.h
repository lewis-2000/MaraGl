#pragma once

#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Shader.h"
#include "Framebuffer.h"

namespace MaraGl
{
    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        void clear(float r, float g, float b, float a);
        void draw();
        void drawToFramebuffer(Framebuffer &framebuffer);

    private:
        VAO m_VAO;
        VBO *m_VBO;
        EBO *m_EBO;
        Shader *m_Shader;
    };
}