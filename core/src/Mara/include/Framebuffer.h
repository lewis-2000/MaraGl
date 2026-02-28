#pragma once

namespace MaraGl
{
    class Framebuffer
    {
    public:
        Framebuffer(unsigned int width, unsigned int height);
        ~Framebuffer();

        void bind();
        void unbind();

        void resize(unsigned int width, unsigned int height);

        unsigned int getColorAttachment() const { return m_ColorAttachment; }
        unsigned int getWidth() const { return m_Width; }
        unsigned int getHeight() const { return m_Height; }

    private:
        void invalidate();

    private:
        unsigned int m_FBO = 0;
        unsigned int m_ColorAttachment = 0;
        unsigned int m_RBO = 0;

        unsigned int m_Width, m_Height;
    };
}