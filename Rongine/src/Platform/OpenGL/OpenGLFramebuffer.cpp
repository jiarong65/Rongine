#include "Rongpch.h"
#include "OpenGLFramebuffer.h"
#include "Rongine/Core/Log.h"
#include "Rongine/Core/Core.h"

#include <glad/glad.h>

namespace Rongine {

	// === 辅助工具函数：判断是否是深度格式 ===
	static bool isDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
		}
		return false;
	}

	// === 辅助工具函数：根据枚举创建纹理 (DSA) ===
	static void createTextures(bool multisampled, uint32_t* outID, uint32_t count)
	{
		glCreateTextures(GL_TEXTURE_2D, count, outID);
		// 这里的 multisampled 暂时没用，为了以后抗锯齿预留
	}

	static void bindTexture(bool multisampled, uint32_t id)
	{
		glBindTexture(GL_TEXTURE_2D, id);
	}

	// === 辅助工具函数：配置纹理参数 ===
	static void attachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
	{
		// 1. 分配显存
		glTextureStorage2D(id, 1, internalFormat, width, height);

		// 2. 设置过滤参数
		// 注意：整数纹理 (RED_INTEGER) 必须使用 NEAREST，不能线性插值！
		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// 3. 绑定到 FBO
		// glNamedFramebufferTexture 是 OpenGL 4.5 DSA
		// 如果我们把 FBO ID 传进去，还需要加上 FBO 参数，这里为了通用性
		// 我们在 invalidate 外部统一做 glNamedFramebufferTexture
	}

	// ---------------------------------------------------------

	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
		: m_specification(spec)
	{
		// 1. 分离颜色附件和深度附件的配置
		for (auto spec : m_specification.Attachments.Attachments)
		{
			if (!isDepthFormat(spec.TextureFormat))
				m_colorAttachmentSpecs.emplace_back(spec);
			else
				m_depthAttachmentSpec = spec;
		}

		invalidate();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		glDeleteFramebuffers(1, &m_rendererID);
		glDeleteTextures(m_colorAttachments.size(), m_colorAttachments.data());
		glDeleteTextures(1, &m_depthAttachment);
	}

	void OpenGLFramebuffer::invalidate()
	{
		if (m_rendererID)
		{
			glDeleteFramebuffers(1, &m_rendererID);
			glDeleteTextures(m_colorAttachments.size(), m_colorAttachments.data());
			glDeleteTextures(1, &m_depthAttachment);

			m_colorAttachments.clear();
			m_depthAttachment = 0;
		}

		// 创建 FBO
		glCreateFramebuffers(1, &m_rendererID);

		bool multisample = m_specification.samples > 1;

		// --- 1. 处理颜色附件 ---
		if (m_colorAttachmentSpecs.size())
		{
			// 调整 vector 大小并创建 Texture IDs
			m_colorAttachments.resize(m_colorAttachmentSpecs.size());
			createTextures(multisample, m_colorAttachments.data(), m_colorAttachments.size());

			for (size_t i = 0; i < m_colorAttachments.size(); i++)
			{
				// 根据格式进行不同的显存分配
				switch (m_colorAttachmentSpecs[i].TextureFormat)
				{
				case FramebufferTextureFormat::RGBA8:
					attachColorTexture(m_colorAttachments[i], m_specification.samples, GL_RGBA8, GL_RGBA, m_specification.width, m_specification.height, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					// 注意：R32I 是 32位整数，Format 是 RED_INTEGER
					// 重要：对于整数纹理，必须覆盖过滤参数为 Nearest，否则无法读取
					glTextureStorage2D(m_colorAttachments[i], 1, GL_R32I, m_specification.width, m_specification.height);
					glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					break;
				}

				// 将纹理挂载到 FBO 的 COLOR_ATTACHMENT[i]
				glNamedFramebufferTexture(m_rendererID, GL_COLOR_ATTACHMENT0 + i, m_colorAttachments[i], 0);
			}
		}

		// --- 2. 处理深度附件 ---
		if (m_depthAttachmentSpec.TextureFormat != FramebufferTextureFormat::None)
		{
			createTextures(multisample, &m_depthAttachment, 1);
			glTextureStorage2D(m_depthAttachment, 1, GL_DEPTH24_STENCIL8, m_specification.width, m_specification.height);
			glNamedFramebufferTexture(m_rendererID, GL_DEPTH_STENCIL_ATTACHMENT, m_depthAttachment, 0);
		}

		// --- 3. 告诉 OpenGL 我们要画到哪些附件上 (DrawBuffers) ---
		if (m_colorAttachments.size() > 1)
		{
			RONG_CORE_ASSERT(m_colorAttachments.size() <= 4, "Rongine only supports 4 color attachments!");
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

			// DSA 版本的 DrawBuffers
			glNamedFramebufferDrawBuffers(m_rendererID, m_colorAttachments.size(), buffers);
		}
		else if (m_colorAttachments.empty())
		{
			// 只有深度的情况
			glNamedFramebufferDrawBuffer(m_rendererID, GL_NONE);
		}

		// 检查完整性
		if (glCheckNamedFramebufferStatus(m_rendererID, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			RONG_CORE_ERROR("Framebuffer is incomplete!");
		}
	}

	void OpenGLFramebuffer::bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
		glViewport(0, 0, m_specification.width, m_specification.height);
	}

	void OpenGLFramebuffer::unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	uint32_t OpenGLFramebuffer::getColorAttachmentRendererID(uint32_t index) const
	{
		RONG_CORE_ASSERT(index < m_colorAttachments.size(), "Index out of bounds!");
		return m_colorAttachments[index];
	}

	void OpenGLFramebuffer::resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || width > 8192 || height > 8192) return;
		m_specification.width = width;
		m_specification.height = height;
		invalidate();
	}

	// 读取像素 ID
	int OpenGLFramebuffer::readPixel(uint32_t attachmentIndex, int x, int y)
	{
		RONG_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(),"attachmentIndex > m_colorAttachments.size()");

		// 这里的 ReadBuffer 设置为我们要读取的那个 attachment
		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

		int pixelData;
		// 读取 x, y 处 1x1 大小的像素，格式为 RED_INTEGER, 类型为 INT
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

		return pixelData;
	}

	// 清空附件
	void OpenGLFramebuffer::clearAttachment(uint32_t attachmentIndex, int value)
	{
		RONG_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(),"attachmentIndex > m_colorAttachments.size()");

		auto& spec = m_colorAttachmentSpecs[attachmentIndex];

		glClearTexImage(m_colorAttachments[attachmentIndex], 0,
			GL_RED_INTEGER, GL_INT, &value);
	}
}