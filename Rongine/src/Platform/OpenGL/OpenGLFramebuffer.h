#pragma once
#include "Rongine/Renderer/Framebuffer.h"

namespace Rongine {


	class OpenGLFramebuffer:public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		void invalidate();

		virtual void bind() override;
		virtual void unbind() override;

		virtual uint32_t getColorAttachmentRendererID(uint32_t index=0) const override;
		virtual const FramebufferSpecification& getSpecification() const override { return m_specification; }

		virtual void resize(uint32_t width, uint32_t height) override;

		virtual int readPixel(uint32_t attachmentIndex, int x, int y) override;
		virtual void clearAttachment(uint32_t attachmentIndex, int value) override;

	private:
		uint32_t m_rendererID;
		FramebufferSpecification m_specification;

		std::vector<uint32_t> m_colorAttachments;
		uint32_t  m_depthAttachment;

		std::vector<FramebufferTextureSpecification> m_colorAttachmentSpecs;
		FramebufferTextureSpecification m_depthAttachmentSpec = FramebufferTextureFormat::None;
	};

}


