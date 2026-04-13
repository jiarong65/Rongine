#include "Rongpch.h"
#include "OpenGLPipelineState.h"
#include <glad/glad.h>

namespace Rongine {

	static GLenum ToGLBlendFactor(BlendFactor factor)
	{
		switch (factor)
		{
		case BlendFactor::Zero:                return GL_ZERO;
		case BlendFactor::One:                 return GL_ONE;
		case BlendFactor::SrcAlpha:            return GL_SRC_ALPHA;
		case BlendFactor::OneMinusSrcAlpha:    return GL_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha:            return GL_DST_ALPHA;
		case BlendFactor::OneMinusDstAlpha:    return GL_ONE_MINUS_DST_ALPHA;
		}
		return GL_ONE;
	}

	static GLenum ToGLDepthFunc(DepthFunc func)
	{
		switch (func)
		{
		case DepthFunc::Less:         return GL_LESS;
		case DepthFunc::LessEqual:    return GL_LEQUAL;
		case DepthFunc::Equal:        return GL_EQUAL;
		case DepthFunc::Greater:      return GL_GREATER;
		case DepthFunc::GreaterEqual: return GL_GEQUAL;
		case DepthFunc::Always:       return GL_ALWAYS;
		case DepthFunc::Never:        return GL_NEVER;
		}
		return GL_LESS;
	}

	OpenGLPipelineState::OpenGLPipelineState(const PipelineStateDesc& desc)
		: m_desc(desc)
	{
	}

	void OpenGLPipelineState::bind() const
	{
		// Blend
		if (m_desc.Blend.Enabled)
		{
			glEnable(GL_BLEND);
			glBlendFunc(ToGLBlendFactor(m_desc.Blend.SrcFactor),
			            ToGLBlendFactor(m_desc.Blend.DstFactor));
		}
		else
		{
			glDisable(GL_BLEND);
		}

		// Depth
		if (m_desc.Depth.TestEnabled)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(ToGLDepthFunc(m_desc.Depth.CompareFunc));
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
		glDepthMask(m_desc.Depth.WriteEnabled ? GL_TRUE : GL_FALSE);

		// Rasterizer - Culling
		if (m_desc.Rasterizer.Culling != CullFace::None)
		{
			glEnable(GL_CULL_FACE);
			switch (m_desc.Rasterizer.Culling)
			{
			case CullFace::Front:        glCullFace(GL_FRONT); break;
			case CullFace::Back:         glCullFace(GL_BACK); break;
			case CullFace::FrontAndBack: glCullFace(GL_FRONT_AND_BACK); break;
			default: break;
			}
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}

		// Rasterizer - Fill mode
		switch (m_desc.Rasterizer.FillMode)
		{
		case PolygonMode::Fill:  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
		case PolygonMode::Line:  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
		case PolygonMode::Point: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
		}

		// Rasterizer - Line width
		glLineWidth(m_desc.Rasterizer.LineWidth);

		// Rasterizer - Polygon offset
		if (m_desc.Rasterizer.PolygonOffsetEnabled)
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(m_desc.Rasterizer.PolygonOffsetFactor,
			                m_desc.Rasterizer.PolygonOffsetUnits);
		}
		else
		{
			glDisable(GL_POLYGON_OFFSET_FILL);
		}

		// Shader
		if (m_desc.Shader)
			m_desc.Shader->bind();
	}

	void OpenGLPipelineState::unbind() const
	{
		if (m_desc.Shader)
			m_desc.Shader->unbind();

		// 恢复默认状态
		glDisable(GL_POLYGON_OFFSET_FILL);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthMask(GL_TRUE);
		glLineWidth(1.0f);
	}

}
