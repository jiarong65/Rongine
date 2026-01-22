#pragma once
#include "Rongine/Renderer/Texture.h"
#include "stb_image.h"

#include <glad/glad.h>

namespace Rongine {
	class OpenGLTexture2D:public Texture2D
	{
	public:
		OpenGLTexture2D(uint32_t width,uint32_t height);
		OpenGLTexture2D(const std::string& path);
		OpenGLTexture2D(const TextureSpecification& specification);
		virtual ~OpenGLTexture2D() override;

		virtual uint32_t getWidth() const override { return m_width; }
		virtual uint32_t getHeight() const override { return m_height; }

		virtual uint32_t getRendererID() const override { return m_rendererID; }

		virtual void bind(uint32_t slot=0) override;

		virtual void setData(void* data, uint32_t size) override;

		virtual bool operator==(const Texture& other) const override {
			return m_rendererID == ((OpenGLTexture2D&)other).m_rendererID;
		}
	private:
		TextureSpecification m_specification;

		std::string m_path;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_rendererID;

		GLenum m_internalFormat, m_dataFormat;
	};

}


