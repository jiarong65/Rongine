#include "Rongpch.h"
#include "RenderPass.h"
#include "Rongine/Renderer/RenderCommand.h"

namespace Rongine {

	RenderPass::RenderPass(const RenderPassSpec& spec)
		: m_spec(spec)
	{
	}

	void RenderPass::begin()
	{
		if (m_spec.TargetFramebuffer)
			m_spec.TargetFramebuffer->bind();

		if (m_spec.ClearColorBuffer)
			RenderCommand::setColor(m_spec.ClearColor);

		if (m_spec.ClearColorBuffer || m_spec.ClearDepthBuffer)
			RenderCommand::clear();

		if (m_spec.ClearAttachmentIndex >= 0 && m_spec.TargetFramebuffer)
			m_spec.TargetFramebuffer->clearAttachment(m_spec.ClearAttachmentIndex, m_spec.ClearAttachmentValue);
	}

	void RenderPass::end()
	{
		if (m_spec.TargetFramebuffer)
			m_spec.TargetFramebuffer->unbind();
	}

}
