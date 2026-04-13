#pragma once
#include "Rongine/Core/Core.h"
#include "Rongine/Renderer/Shader.h"

namespace Rongine {

	enum class BlendFactor
	{
		Zero, One,
		SrcAlpha, OneMinusSrcAlpha,
		DstAlpha, OneMinusDstAlpha
	};

	enum class DepthFunc
	{
		Less, LessEqual, Equal, Greater, GreaterEqual, Always, Never
	};

	enum class CullFace
	{
		None, Front, Back, FrontAndBack
	};

	enum class PolygonMode
	{
		Fill, Line, Point
	};

	struct BlendState
	{
		bool Enabled = true;
		BlendFactor SrcFactor = BlendFactor::SrcAlpha;
		BlendFactor DstFactor = BlendFactor::OneMinusSrcAlpha;
	};

	struct DepthState
	{
		bool TestEnabled = true;
		bool WriteEnabled = true;
		DepthFunc CompareFunc = DepthFunc::Less;
	};

	struct RasterState
	{
		CullFace Culling = CullFace::None;
		PolygonMode FillMode = PolygonMode::Fill;
		float LineWidth = 1.0f;
		bool PolygonOffsetEnabled = false;
		float PolygonOffsetFactor = 0.0f;
		float PolygonOffsetUnits = 0.0f;
	};

	struct PipelineStateDesc
	{
		Ref<Shader> Shader;
		BlendState Blend;
		DepthState Depth;
		RasterState Rasterizer;
	};

	class PipelineState
	{
	public:
		virtual ~PipelineState() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual const PipelineStateDesc& getDesc() const = 0;

		static Ref<PipelineState> create(const PipelineStateDesc& desc);
	};

}
