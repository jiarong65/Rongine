#include "Rongpch.h"
#include "Renderer2D.h"
#include "Rongine/Renderer/VertexArray.h"
#include "Rongine/Renderer/Shader.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Renderer/RenderCommand.h"
#include <glm/gtc/matrix_transform.hpp>


namespace Rongine {
	
	struct Renderer2DStorage {
		Ref<VertexArray> QuadVertexArray;
		Ref<Shader> TextureShader;
		Ref<Texture> WhiteTexture;
	};

	Renderer2DStorage* s_data =nullptr;

	void Renderer2D::init()
	{
		s_data = new Renderer2DStorage();

		s_data->QuadVertexArray = VertexArray::create();

		float squareVertices[5* 4] = {
			-0.5f, -0.5f, 0.0f,0.0f,0.0f,
			 0.5f, -0.5f, 0.0f,1.0f,0.0f,
			 0.5f,  0.5f, 0.0f,1.0f,1.0f,
			-0.5f,  0.5f, 0.0f,0.0f,1.0f
		};

		Ref<VertexBuffer> squareVB;
		squareVB = VertexBuffer::create(squareVertices, sizeof(squareVertices));

		BufferLayout layout = {
			{ShaderDataType::Float3,"a_Position"},
			{ShaderDataType::Float2,"a_TexCoord"}
		};
		squareVB->setLayout(layout);
		s_data->QuadVertexArray->addVertexBuffer(squareVB);


		uint32_t squareIndices[6] = { 0,1,2,2,3,0 };

		Ref<IndexBuffer> squareIB;
		squareIB = IndexBuffer::create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t));
		s_data->QuadVertexArray->setIndexBuffer(squareIB);

		s_data->TextureShader = Shader::create("assets/shaders/Texture.glsl");

		s_data->TextureShader->bind();
		s_data->TextureShader->setInt("u_Texture", 0);

		s_data->WhiteTexture = Texture2D::create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_data->WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));
	}

	void Renderer2D::shutdown()
	{
		delete s_data;
	}

	void Renderer2D::beginScene(const OrthographicCamera& camera)
	{
		s_data->TextureShader->bind();
		s_data->TextureShader->setMat4("u_ViewProjection", camera.getViewProjectionMatrix());

	}

	void Renderer2D::endScene()
	{
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		drawQuad(glm::vec3(position, 0.0f), size, color);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		s_data->TextureShader->bind();
		s_data->TextureShader->setFloat4("u_Color", color);

		s_data->WhiteTexture->bind();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f),position)* glm::scale(glm::mat4(1.0f), glm::vec3(size.x,size.y,1.0f));
		s_data->TextureShader->setMat4("u_Transform", transform);

		s_data->QuadVertexArray->bind();
		RenderCommand::drawIndexed(s_data->QuadVertexArray);
	}

	void Renderer2D::drawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4 & tintColor )
	{
		drawQuad(glm::vec3(position, 1.0f), size, texture,tilingFactor,tintColor);
	}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4 & tintColor )
	{
		s_data->TextureShader->bind();
		s_data->TextureShader->setFloat("u_TilingFactor", tilingFactor);
		s_data->TextureShader->setFloat4("u_Color", tintColor);
		
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_data->TextureShader->setMat4("u_Transform", transform);

		s_data->QuadVertexArray->bind();
		texture->bind();
		RenderCommand::drawIndexed(s_data->QuadVertexArray);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		drawRotatedQuad(glm::vec3(position, 1.0f), size, rotation, color);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		s_data->TextureShader->bind();
		s_data->WhiteTexture->bind();
		s_data->TextureShader->setFloat("u_TilingFactor", 1.0f);
		s_data->TextureShader->setFloat4("u_Color", color);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * 
			glm::rotate(glm::mat4(1.0f), rotation, {0.0f,0.0f,1.0f}) *
			glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_data->TextureShader->setMat4("u_Transform", transform);

		s_data->QuadVertexArray->bind();
		RenderCommand::drawIndexed(s_data->QuadVertexArray);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		drawRotatedQuad(glm::vec3(position, 1.0f), size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		s_data->TextureShader->bind();
		s_data->TextureShader->setFloat("u_TilingFactor", tilingFactor);
		s_data->TextureShader->setFloat4("u_Color", tintColor);

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * 
			glm::rotate(glm::mat4(1.0f), rotation, {0.0f,0.0f,1.0f})*
			glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));
		s_data->TextureShader->setMat4("u_Transform", transform);

		s_data->QuadVertexArray->bind();
		texture->bind();
		RenderCommand::drawIndexed(s_data->QuadVertexArray);
	}
}

