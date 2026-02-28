#include "Framebuffer.h"
#include <glad/glad.h>

namespace MaraGl
{
    // Constructor: Creates an off-screen framebuffer object (FBO) with specified dimensions
    // The framebuffer includes a color texture attachment and depth/stencil renderbuffer
    // \param width Width of the framebuffer in pixels
    // \param height Height of the framebuffer in pixels
    Framebuffer::Framebuffer(unsigned int width, unsigned int height)
        : m_Width(width), m_Height(height)
    {
        invalidate();
    }

    // Destructor: Cleans up GPU resources associated with this framebuffer
    // Deletes the FBO, color texture, and depth/stencil renderbuffer
    Framebuffer::~Framebuffer()
    {
        glDeleteFramebuffers(1, &m_FBO);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteRenderbuffers(1, &m_RBO);
    }

    // Creates or recreates the framebuffer and all its attachments
    // Called from constructor and resize() to initialize/reinitialize GPU resources
    // Framebuffer structure:
    //   - Color Attachment: RGBA8 texture for storing rendered output
    //   - Depth/Stencil Attachment: Renderbuffer for depth testing and stencil operations
    void Framebuffer::invalidate()
    {
        // Delete existing GPU resources if framebuffer already exists
        // This allows recreation with new dimensions
        if (m_FBO)
        {
            glDeleteFramebuffers(1, &m_FBO);
            glDeleteTextures(1, &m_ColorAttachment);
            glDeleteRenderbuffers(1, &m_RBO);
        }

        // Create and bind the framebuffer object
        glGenFramebuffers(1, &m_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        // --- Color Attachment Setup ---
        // Create texture to store the rendered color output
        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        // GL_RGBA8: 8-bit RGBA color format (standard for screen-space rendering)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                     m_Width, m_Height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Set texture filtering to linear interpolation
        // GL_LINEAR for minification and magnification ensures smooth sampling
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Attach texture as the color attachment to the framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               m_ColorAttachment, 0);

        // --- Depth/Stencil Attachment Setup ---
        // Create renderbuffer for depth and stencil tests during rendering
        glGenRenderbuffers(1, &m_RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
        // GL_DEPTH24_STENCIL8: 24-bit depth + 8-bit stencil (combined format)
        glRenderbufferStorage(GL_RENDERBUFFER,
                              GL_DEPTH24_STENCIL8,
                              m_Width, m_Height);

        // Attach renderbuffer for depth/stencil operations
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  m_RBO);

        // Unbind framebuffer - switch back to default (screen) framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Activates this framebuffer for rendering
    // All subsequent draw calls will render to this framebuffer's color texture
    // Must be called before rendering operations
    void Framebuffer::bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        // Set viewport to match framebuffer dimensions
        // Ensures graphics are rendered to the full area of the texture
        glViewport(0, 0, m_Width, m_Height);
    }

    // Deactivates this framebuffer and switches back to the default (screen) framebuffer
    // Call when done rendering to this framebuffer to render to screen instead
    void Framebuffer::unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Resizes the framebuffer to new dimensions and recreates all GPU resources
    // Should be called when the viewport/window size changes to keep texture in sync
    // \param width New width in pixels
    // \param height New height in pixels
    void Framebuffer::resize(unsigned int width, unsigned int height)
    {
        m_Width = width;
        m_Height = height;
        // Recreate all GPU resources with new dimensions
        invalidate();
    }
}