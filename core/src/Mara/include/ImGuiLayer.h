#pragma once

#include "Window.h"
#include "Framebuffer.h"

namespace MaraGl
{
    class ImGuiLayer
    {
    public:
        ImGuiLayer(Window &window);
        ~ImGuiLayer();

        void begin();
        void end();

        void renderDockspace(Framebuffer *framebuffer = nullptr);

    private:
        void init();
        void shutdown();

    private:
        Window &m_Window;
        Framebuffer *m_Framebuffer = nullptr;
    };
}