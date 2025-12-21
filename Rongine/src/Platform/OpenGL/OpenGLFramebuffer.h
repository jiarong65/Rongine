#pragma once
#include "Rongine/Renderer/Framebuffer.h"

namespace Rongine {


	class OpenGLFramebuffer:public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FrameSpecification& spec);
		virtual ~OpenGLFramebuffer();

		void invalidate();

		virtual void bind() override;
		virtual void unbind() override;

		virtual uint32_t getColorAttachmentRendererID() const override { return m_colorAttachment; }
		virtual const FrameSpecification& getSpecification() const override { return m_specification; }

	private:
		uint32_t m_rendererID;
		uint32_t m_colorAttachment, m_depthAttachment;

		FrameSpecification m_specification;
	};

}


