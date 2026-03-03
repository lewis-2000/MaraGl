#pragma once

#include "Window.h"
#include "Framebuffer.h"
#include "Renderer.h"

namespace MaraGl
{
    class Renderer; // forward declare

    class ImGuiLayer
    {
    public:
        ImGuiLayer(Window &window);
        ~ImGuiLayer();

        void begin();
        void end();

        void renderDockspace(Framebuffer *framebuffer, Renderer *renderer);

    private:
        void init();
        void shutdown();

    private:
        Window &m_Window;
        Framebuffer *m_Framebuffer = nullptr;
    };
}