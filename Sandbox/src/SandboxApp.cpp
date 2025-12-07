#include "Rongpch.h"
#include <Rongine.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

#include "imgui/imgui.h"

class ExampleLayer :public Rongine::Layer
{
public:
	ExampleLayer()
		:Layer("example"), m_camera(-1.6f, 1.6f, -0.9f, 0.9f)
	{
		m_vertexArray.reset(Rongine::VertexArray::create());

		float vertex[3 * 7] = {
			-0.5f, -0.5f, 0.0f,0.0f,0.0f,1.0f,1.0f,
			 0.5f, -0.5f, 0.0f,0.0f,1.0f,0.0f,1.0f,
			 0.0f,  0.5f, 0.0f,1.0f,0.0f,1.0f,1.0f
		};

		std::shared_ptr<Rongine::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Rongine::VertexBuffer::create(vertex, sizeof(vertex)));

		Rongine::BufferLayout layout = {
			{Rongine::ShaderDataType::Float3,"a_Positon"},
			{Rongine::ShaderDataType::Float4,"a_Color"}
		};
		vertexBuffer->setLayout(layout);
		m_vertexArray->addVertexBuffer(vertexBuffer);


		uint32_t indices[3] = { 0,1,2 };

		std::shared_ptr<Rongine::IndexBuffer> indexBuffer;
		indexBuffer.reset(Rongine::IndexBuffer::create(indices, sizeof(indices) / sizeof(uint32_t)));
		m_vertexArray->setIndexBuffer(indexBuffer);

		//////////////////////////////start
		m_squareVA.reset(Rongine::VertexArray::create());

		float squareVertices[3 * 4] = {
			-0.75f, -0.75f, 0.0f,
			 0.75f, -0.75f, 0.0f,
			 0.75f,  0.75f, 0.0f,
			-0.75f,  0.75f, 0.0f
		};

		std::shared_ptr<Rongine::VertexBuffer> squareVB;
		squareVB.reset(Rongine::VertexBuffer::create(squareVertices, sizeof(squareVertices)));

		squareVB->setLayout({
			{ Rongine::ShaderDataType::Float3, "a_Position" }
			});
		m_squareVA->addVertexBuffer(squareVB);


		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };

		std::shared_ptr<Rongine::IndexBuffer> squareIB;
		squareIB.reset(Rongine::IndexBuffer::create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
		m_squareVA->setIndexBuffer(squareIB);

		//////////////////////////////////end

		std::string vertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec4 a_Color;

			uniform mat4 u_ViewProjection;

			out vec3 v_Position;
			out vec4 v_Color;

			void main()
			{
				v_Position = a_Position;
				v_Color=a_Color;
				gl_Position = u_ViewProjection*vec4(a_Position,1.0f);	
			}
		)";

		std::string fragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_Position;
			in vec4 v_Color;

			void main()
			{
				color = v_Color;
			}
		)";

		m_shader.reset(new Rongine::Shader(vertexSrc, fragmentSrc));

		/////////////////////////////////start
		std::string blueShaderVertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;

			uniform mat4 u_ViewProjection;

			out vec3 v_Position;

			void main()
			{
				v_Position = a_Position;
				gl_Position = u_ViewProjection*vec4(a_Position,1.0f);	
			}
		)";

		std::string blueShaderFragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;

			in vec3 v_Position;

			void main()
			{
				color = vec4(0.2, 0.3, 0.8, 1.0);
			}
		)";

		m_blueShader.reset(new Rongine::Shader(blueShaderVertexSrc, blueShaderFragmentSrc));
		/////////////////////////////////end
	}

	void onUpdate(Rongine::Timestep ts) override
	{
		RONG_CLIENT_TRACE("Timestep is {0} ms", ts.getMilliseconds());

		if (Rongine::Input::isKeyPressed(Rongine::Key::Left))
			m_cameraPosition.x -= m_cameraMoveSpeed * ts;
		else if (Rongine::Input::isKeyPressed(Rongine::Key::Right))
			m_cameraPosition.x += m_cameraMoveSpeed * ts;
		if (Rongine::Input::isKeyPressed(Rongine::Key::Up))
			m_cameraPosition.y += m_cameraMoveSpeed * ts;
		else if(Rongine::Input::isKeyPressed(Rongine::Key::Down))
			m_cameraPosition.y -= m_cameraMoveSpeed * ts;

		if(Rongine::Input::isKeyPressed(Rongine::Key::A))
			m_cameraRotation -= m_cameraRotationSpeed * ts;
		else if(Rongine::Input::isKeyPressed(Rongine::Key::D))
			m_cameraRotation += m_cameraRotationSpeed * ts;

		Rongine::RenderCommand::setColor({ 0.1f, 0.1f, 0.1f, 1 });
		Rongine::RenderCommand::clear();

		m_camera.setPosition(m_cameraPosition);
		m_camera.setRotation(m_cameraRotation);

		Rongine::Renderer::beginScene(m_camera);

		//////////////////start
		Rongine::Renderer::submit(m_blueShader, m_squareVA);
		//////////////////end

		Rongine::Renderer::submit(m_shader, m_vertexArray);

		Rongine::Renderer::endScene();
	}

	void onEvent(Rongine::Event& event)
	{
		//RONG_CLIENT_TRACE(event.toString());
		if (event.getEventType() == Rongine::EventType::KeyPressed)
		{
			Rongine::KeyPressedEvent& e = (Rongine::KeyPressedEvent&)event;
			if (e.getKeyCode() == Rongine::Key::A)
			{
				RONG_CLIENT_TRACE("A key event received(EVENT)!");
			}
		}
	}

	void onImGuiRender()
	{
		ImGui::Begin("test");
		ImGui::Text("hello world!!");
		ImGui::End();
	}

private:
	std::shared_ptr<Rongine::Shader> m_shader;
	std::shared_ptr<Rongine::VertexArray> m_vertexArray;

	std::shared_ptr<Rongine::Shader> m_blueShader;
	std::shared_ptr<Rongine::VertexArray> m_squareVA;

	Rongine::OrthographicCamera m_camera;

	glm::vec3 m_cameraPosition = { 0.0f,0.0f,0.0f };
	float m_cameraMoveSpeed = 3.0f;
	float m_cameraRotation = 0.0f;
	float m_cameraRotationSpeed = 90.0f;
};

class Sandbox :public Rongine::Application {
public:
	Sandbox() {
		pushLayer(new ExampleLayer());
		pushOverLayer(new Rongine::ImGuiLayer());
	}
	~Sandbox() {

	}
};

Rongine::Application* Rongine::createApplication()
{
	return new Sandbox;
}