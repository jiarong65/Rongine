#include "Rongpch.h"
#include "OpenGLFramebuffer.h"
#include "Rongine/Core/Log.h" // 确保包含 Log 头文件

#include <glad/glad.h>

namespace Rongine {

    OpenGLFramebuffer::OpenGLFramebuffer(const FrameSpecification& spec)
        : m_specification(spec), m_rendererID(0), m_colorAttachment(0), m_depthAttachment(0)
    {
        invalidate();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        glDeleteFramebuffers(1, &m_rendererID);
        glDeleteTextures(1, &m_colorAttachment);
        glDeleteTextures(1, &m_depthAttachment);
    }

    void OpenGLFramebuffer::invalidate()
    {
        if (m_rendererID)
        {
            glDeleteFramebuffers(1, &m_rendererID);
            glDeleteTextures(1, &m_colorAttachment);
            glDeleteTextures(1, &m_depthAttachment);

            m_colorAttachment = 0;
            m_depthAttachment = 0;
            m_rendererID = 0;
        }

        // ================= OpenGL 4.5 DSA 写法 =================

        // 1. 创建 Framebuffer (不需要 Bind)
        glCreateFramebuffers(1, &m_rendererID);

        // 2. 创建颜色附件 (不需要 Bind)
        glCreateTextures(GL_TEXTURE_2D, 1, &m_colorAttachment);

        // 使用 glTextureStorage2D 分配显存
        // 这里的 '1' 是 Mipmap Levels，这里只分配 1 层
        glTextureStorage2D(m_colorAttachment, 1, GL_RGBA8, m_specification.width, m_specification.height);

        // 设置纹理参数
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_colorAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // 将纹理绑定到 Framebuffer
        glNamedFramebufferTexture(m_rendererID, GL_COLOR_ATTACHMENT0, m_colorAttachment, 0);

        // 3. 创建深度附件 (不需要 Bind)
        glCreateTextures(GL_TEXTURE_2D, 1, &m_depthAttachment);
        glTextureStorage2D(m_depthAttachment, 1, GL_DEPTH24_STENCIL8, m_specification.width, m_specification.height);

        // 将深度纹理绑定到 Framebuffer
        glNamedFramebufferTexture(m_rendererID, GL_DEPTH_STENCIL_ATTACHMENT, m_depthAttachment, 0);

        // 4. 检查完整性 (直接检查 ID)
        if (glCheckNamedFramebufferStatus(m_rendererID, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            RONG_CORE_ERROR("Framebuffer is incomplete!");
        }

        // DSA 不需要解绑，因为我们全程没有 Bind 任何东西。
        // 但是为了安全起见，或者如果不希望影响后续非 DSA 代码，保持原来的 bind/unbind 逻辑在渲染时使用即可。
    }

    void OpenGLFramebuffer::bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
        // 设置 Viewport 通常和 Bind Framebuffer 配套
        glViewport(0, 0, m_specification.width, m_specification.height);
    }

    void OpenGLFramebuffer::unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0 || width > 8192 || height > 8192)
        {
            RONG_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
            return;
        }

        m_specification.width = width;
        m_specification.height = height;

        invalidate();
    }
}