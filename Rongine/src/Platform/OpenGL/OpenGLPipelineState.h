#pragma once
#include "Rongine/Renderer/PipelineState.h"

namespace Rongine {

	class OpenGLPipelineState : public PipelineState
	{
	public:
		OpenGLPipelineState(const PipelineStateDesc& desc);
		virtual ~OpenGLPipelineState() = default;

		virtual void bind() const override;
		virtual void unbind() const override;

		virtual const PipelineStateDesc& getDesc() const override { return m_desc; }

	private:
		PipelineStateDesc m_desc;
	};

}
