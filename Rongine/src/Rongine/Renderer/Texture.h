#pragma once
#include "Rongine/Core/Core.h"

namespace Rongine {

	enum class ImageFormat
	{
		None = 0,
		R8,
		RGB8,
		RGBA8,
		RGBA32F // 32位浮点，光线追踪必须用这个！
	};

	struct TextureSpecification
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;
	};


	class Texture
	{
	public:
		virtual ~Texture() = default;

		virtual uint32_t getWidth() const = 0;
		virtual uint32_t getHeight() const = 0;
		virtual uint32_t getRendererID()const = 0;

		virtual void bind(uint32_t slot=0) = 0;

		virtual void setData(void* data, uint32_t size) = 0;

		virtual bool operator==(const Texture& texture) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		static Ref<Texture2D> create(uint32_t width, uint32_t height);
		static Ref<Texture2D> create(const std::string& path);

		static Ref<Texture2D> create(const TextureSpecification& specification);
	};


}

