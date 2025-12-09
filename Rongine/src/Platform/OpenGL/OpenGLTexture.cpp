#include "Rongpch.h"
#include "OpenGLTexture.h"
#include "Rongine/Log.h"
#include <glad/glad.h>

namespace Rongine {

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
		:m_path(path)
	{
		stbi_set_flip_vertically_on_load(1);

		int width, height, channels;
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels,0);
		RONG_CORE_ASSERT(data, "Failed to load image!");

		m_width = width;
		m_height = height;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
		glTextureStorage2D(m_rendererID, 1, GL_RGB8, m_width, m_height);

		glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureSubImage2D(m_rendererID, 0, 0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, data);

		stbi_image_free(data);
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_rendererID);
	}

	void OpenGLTexture2D::bind(uint32_t slot)
	{
		glBindTextureUnit(slot, m_rendererID);
	}

}
