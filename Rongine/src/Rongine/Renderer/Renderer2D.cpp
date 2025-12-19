#include "Rongpch.h"
#include "Renderer2D.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Shader.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Renderer/RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>


namespace Rongine {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
	};

	struct Renderer2DData
	{
		const uint32_t MaxQuads = 10000;
		const uint32_t MaxVertices = MaxQuads * 4;
		const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;

		Ref<VertexArray> QuadVertexArray;
		Ref<IndexBuffer> QuadIndexBuffer;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> TextureShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotsIndex = 1;//0=whiteTexture;

		glm::vec4 QuadVertexPositions[4];
	};
	

	static Renderer2DData s_data ;

	void Renderer2D::init()
	{
		s_data.QuadVertexArray = VertexArray::create();

		s_data.QuadVertexBuffer = VertexBuffer::create(s_data.MaxVertices*sizeof(QuadVertex));

		s_data.QuadVertexBufferBase = new QuadVertex[s_data.MaxVertices];

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_Position"},
			{ShaderDataType::Float4,"a_Color"},
			{ShaderDataType::Float2,"a_TexCoord"},
			{ShaderDataType::Float,"a_TexIndex"},
			{ShaderDataType::Float,"a_TilingFactor"}
		};
		s_data.QuadVertexBuffer->setLayout(layout);
		s_data.QuadVertexArray->addVertexBuffer(s_data.QuadVertexBuffer);


		uint32_t squareIndices[6] = { 0,1,2,2,3,0 };

		uint32_t* quadIndices = new uint32_t[s_data.MaxIndices];
		uint32_t offset = 0;

		for (uint32_t i = 0; i < s_data.MaxIndices; i+=6)
		{
			quadIndices[i] = offset;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		s_data.QuadIndexBuffer = IndexBuffer::create(quadIndices, s_data.MaxIndices);
		s_data.QuadVertexArray->setIndexBuffer(s_data.QuadIndexBuffer);

		delete[] quadIndices;

		s_data.TextureShader = Shader::create("assets/shaders/Texture.glsl");

		s_data.TextureShader->bind();
		s_data.TextureShader->setInt("u_Texture", 0);

		s_data.WhiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_data.WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_data.MaxTextureSlots];
		for (int i = 0; i < s_data.MaxTextureSlots; i++)
			samplers[i] = i;

		s_data.TextureShader->setIntArray("u_Textures", samplers,s_data.MaxTextureSlots);

		s_data.TextureSlots[0] = s_data.WhiteTexture;

		s_data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_data.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		s_data.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		s_data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };
	}

	void Renderer2D::shutdown()
	{
		delete[] s_data.QuadVertexBufferBase;
	}

	void Renderer2D::beginScene(const OrthographicCamera& camera)
	{
		s_data.TextureShader->bind();
		s_data.TextureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

		s_data.QuadIndexCount = 0;
		s_data.QuadVertexBufferPtr = s_data.QuadVertexBufferBase;
		s_data.TextureSlotsIndex = 1;
	}

	void Renderer2D::endScene()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_data.QuadVertexBufferPtr - (uint8_t*)s_data.QuadVertexBufferBase);
		s_data.QuadVertexBuffer->setData(s_data.QuadVertexBufferBase, dataSize);

		flush();
	}

	void Renderer2D::flush()
	{
		s_data.QuadVertexArray->bind();
		s_data.TextureShader->bind();

		for (uint32_t i = 0; i < s_data.TextureSlotsIndex; i++)
			s_data.TextureSlots[i]->bind(i);
		RenderCommand::drawIndexed(s_data.QuadVertexArray, s_data.QuadIndexCount);
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		drawQuad(glm::vec3(position, 0.0f), size, color);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		const float texIndex = 0.0f;
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
			glm::scale(glm::mat4(1.0f), {size.x,size.y,1.0f});

		s_data.QuadVertexBufferPtr->Position = transform* s_data.QuadVertexPositions[0];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 2：右下
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[1];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 3：右上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[2];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 4：左上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[3];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		s_data.QuadIndexCount += 6;

		//s_data.TextureShader->bind();
		//s_data.TextureShader->setFloat4("u_Color", color);
		//	  
		//s_data.WhiteTexture->bind();

		//glm::mat4 transform = glm::translate(glm::mat4(1.0f),position)* glm::scale(glm::mat4(1.0f), glm::vec3(size.x,size.y,1.0f));
		//s_data.TextureShader->setMat4("u_Transform", transform);

		//s_data.QuadVertexArray->bind();
		//RenderCommand::drawIndexed(s_data.QuadVertexArray);
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4 & tintColor )
	{
		drawQuad(glm::vec3(position, 1.0f), size, texture,tilingFactor,tintColor);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4 & tintColor )
	{
		float texIndex = 0.0f;
		for (uint32_t i = 0; i < s_data.TextureSlotsIndex; i++)
		{
			if (*texture.get() == *s_data.TextureSlots[i].get()) {
				texIndex = (float)i;
				break;
			}
		}

		if (texIndex == 0.0f)
		{
			texIndex = (float)s_data.TextureSlotsIndex;
			s_data.TextureSlots[s_data.TextureSlotsIndex] = texture;
			s_data.TextureSlotsIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
			glm::scale(glm::mat4(1.0f), { size.x,size.y,1.0f });

		s_data.QuadVertexBufferPtr->Position = transform* s_data.QuadVertexPositions[0];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 2：右下
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[1];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 3：右上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[2];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 4：左上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[3];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		s_data.QuadIndexCount += 6;
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		drawRotatedQuad(glm::vec3(position, 1.0f), size, rotation, color);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		const float texIndex = 0.0f;
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation), {0.0f,0.0f,1.0f})*
			glm::scale(glm::mat4(1.0f), { size.x,size.y,1.0f });

		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[0];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 2：右下
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[1];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 3：右上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[2];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 4：左上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[3];
		s_data.QuadVertexBufferPtr->Color = color;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		s_data.QuadIndexCount += 6;
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		drawRotatedQuad(glm::vec3(position, 1.0f), size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		float texIndex = 0.0f;
		for (uint32_t i = 0; i < s_data.TextureSlotsIndex; i++)
		{
			if (*texture.get() == *s_data.TextureSlots[i].get()) {
				texIndex = (float)i;
				break;
			}
		}

		if (texIndex == 0.0f)
		{
			texIndex = (float)s_data.TextureSlotsIndex;
			s_data.TextureSlots[s_data.TextureSlotsIndex] = texture;
			s_data.TextureSlotsIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) *
			glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f,0.0f,1.0f }) *
			glm::scale(glm::mat4(1.0f), { size.x,size.y,1.0f });

		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[0];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 2：右下
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[1];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 3：右上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[2];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		// 顶点 4：左上
		s_data.QuadVertexBufferPtr->Position = transform * s_data.QuadVertexPositions[3];
		s_data.QuadVertexBufferPtr->Color = tintColor;
		s_data.QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_data.QuadVertexBufferPtr++;

		s_data.QuadIndexCount += 6;
	}
}

