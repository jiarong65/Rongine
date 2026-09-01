module;
#include <cstdint>

export module Rongine.TextureData;

export namespace Rongine {

	enum class ImageFormat
	{
		None = 0,
		R8,
		RGB8,
		RGBA8,
		RGBA32F // 32-bit float, required by raytracing
	};

	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;
	};
}
