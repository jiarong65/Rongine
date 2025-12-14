#include "Rongpch.h"
#include "Sandbox2D.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Rongine/Core/Input.h"
#include "Rongine/Core/KeyCodes.h"
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"

Sandbox2D::Sandbox2D()
	:Layer("Sandbox2D"), m_cameraContorller(1280.0f/720.0f)
{
}

void Sandbox2D::onAttach()
{
	m_vertexArray = Rongine::VertexArray::create();

	float vertices[3 * 4] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		 0.5f,  0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f
	};

	Rongine::Ref<Rongine::VertexBuffer> vertexBuffer;
	vertexBuffer=Rongine::VertexBuffer::create(vertices, sizeof(vertices));

	Rongine::BufferLayout layout = {
		{Rongine::ShaderDataType::Float3,"a_Positon"}
	};
	vertexBuffer->setLayout(layout);
	m_vertexArray->addVertexBuffer(vertexBuffer);


	uint32_t indices[6] = { 0,1,2,2,3,0 };

	Rongine::Ref<Rongine::IndexBuffer> indexBuffer;
	indexBuffer=Rongine::IndexBuffer::create(indices, sizeof(indices) / sizeof(uint32_t));
	m_vertexArray->setIndexBuffer(indexBuffer);

	m_shader = Rongine::Shader::create("assets/shaders/FlatColor.glsl");

}

void Sandbox2D::onDetach()
{

}

void Sandbox2D::onUpdate(Rongine::Timestep ts)
{
	m_cameraContorller.onUpdate(ts);

	if (Rongine::Input::isKeyPressed(Rongine::Key::J))
		m_squarePosition.x -= m_squareMovedSpeed * ts;
	else if (Rongine::Input::isKeyPressed(Rongine::Key::L))
		m_squarePosition.x += m_squareMovedSpeed * ts;
	if (Rongine::Input::isKeyPressed(Rongine::Key::I))
		m_squarePosition.y += m_squareMovedSpeed * ts;
	else if (Rongine::Input::isKeyPressed(Rongine::Key::K))
		m_squarePosition.y -= m_squareMovedSpeed * ts;

	Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
	Rongine::RenderCommand::clear();

	Rongine::Renderer::beginScene(m_cameraContorller.getCamera());

	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

	Rongine::Ref<Rongine::OpenGLShader> shader_ptr = std::dynamic_pointer_cast<Rongine::OpenGLShader>(m_shader);
	shader_ptr->bind();
	shader_ptr->uploadUniformFloat4("u_Color", m_squareColor);

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 20; x++)
		{
			glm::vec3 gridOffset(x * 0.11f, y * 0.11f, 0.0f);
			glm::vec3 pos = m_squarePosition + gridOffset;
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
			Rongine::Renderer::submit(std::dynamic_pointer_cast<Rongine::OpenGLShader>(m_shader), m_vertexArray, transform);
		}
	}

	Rongine::Renderer::endScene();
}

void Sandbox2D::onImGuiRender()
{
	ImGui::Begin("Setting");
	ImGui::ColorEdit3("Square Color", glm::value_ptr(m_squareColor));
	ImGui::End();
}

void Sandbox2D::onEvent(Rongine::Event& e)
{
	m_cameraContorller.onEvent(e);
}
