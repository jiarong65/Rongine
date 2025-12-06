#include "Rongpch.h"
#include "Application.h"
#include "ApplicationEvent.h"
#include "Log.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/Windows/WindowsInput.h"
#include "Input.h"

namespace Rongine {
	Application* Application::s_instance = nullptr;

	static GLenum shaderDateTypetoOpenGLBaseType(const Rongine::ShaderDataType& type)
	{
		switch (type)
		{
		case ShaderDataType::Float: 
		case ShaderDataType::Float2:
		case ShaderDataType::Float3:
		case ShaderDataType::Float4: 
		case ShaderDataType::Mat3: 
		case ShaderDataType::Mat4: return GL_FLOAT;
		case ShaderDataType::Int: 
		case ShaderDataType::Int2: 
		case ShaderDataType::Int3: 
		case ShaderDataType::Int4: return GL_INT;
		case ShaderDataType::Bool: return GL_BOOL;
		}
		RONG_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	Application::Application() {
		RONG_CORE_ASSERT(!s_instance, "Application already exists!");
		s_instance = this;
		m_window = std::unique_ptr<Window> (Window::create());
		m_window->setEventCallBack(RONG_BIND_EVENT_FN(onEvent));
		m_imguiLayer = new ImGuiLayer();

		glGenVertexArrays(1, &m_vertexArray);
		glBindVertexArray(m_vertexArray);

		float vertex[3 * 7] = {
			-0.5f, -0.5f, 0.0f,0.0f,0.0f,1.0f,1.0f,
			 0.5f, -0.5f, 0.0f,0.0f,1.0f,0.0f,1.0f,
			 0.0f,  0.5f, 0.0f,1.0f,0.0f,1.0f,1.0f
		};

		m_vertexBuffer.reset(VertexBuffer::create(vertex, sizeof(vertex)));

		/*glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), nullptr);*/

		BufferLayout layout={
			{ShaderDataType::Float3,"a_Positon"},
			{ShaderDataType::Float4,"a_Color"}
		};

		uint32_t index = 0;
		m_vertexBuffer->setLayout(layout);
		for (const auto& element : m_vertexBuffer->getLayout())
		{
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(
				index,
				element.getComponentCount(),
				shaderDateTypetoOpenGLBaseType(element.type),
				element.normalized?GL_TRUE:GL_FALSE,
				m_vertexBuffer->getLayout().getStride(),
				(void *)element.offset
			);
			index++;
		}

		uint32_t indices[3] = { 0,1,2 };
		m_indexBuffer.reset(IndexBuffer::create(indices, sizeof(indices) / sizeof(uint32_t)));

		std::string vertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec4 a_Color;

			out vec3 v_Position;
			out vec4 v_Color;

			void main()
			{
				v_Position = a_Position;
				v_Color=a_Color;
				gl_Position = vec4(a_Position, 1.0);	
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

		m_shader.reset(new Shader(vertexSrc, fragmentSrc));
	}

	Application::~Application() {

	}

	void Application::onEvent(Event& event) {
		RONG_CORE_INFO( event.toString());
		EventDispatcher dispatcher(event);

		dispatcher.dispatch<WindowCloseEvent>(RONG_BIND_EVENT_FN(onWindowClose));

		for (auto& it = m_layerStack.rbegin(); it != m_layerStack.rend();++it)
		{
			if (event.handled)
				break;
			(*it)->onEvent(event);
		}
	}

	void Application::pushLayer(Layer* layer)
	{
		m_layerStack.pushLayer(layer);
	}

	void Application::pushOverLayer(Layer* layer)
	{
		m_layerStack.pushOverLayer(layer);
	}

	void Application::run() {
		WindowResizeEvent e(1280,720);
		RONG_CLIENT_TRACE( e.toString());

		while (m_running) {
			glClearColor(0.2f,0.2f,0.2f,0.8f);
			glClear(GL_COLOR_BUFFER_BIT);

			m_shader->bind();

			glBindVertexArray(m_vertexArray);
			glDrawElements(GL_TRIANGLES,m_indexBuffer->getCount(),GL_UNSIGNED_INT,nullptr);

			for (Layer* layer : m_layerStack)
				layer->onUpdate();

			m_imguiLayer->begin();
			for (Layer* layer : m_layerStack)
				layer->onImGuiRender();
			m_imguiLayer->end();

			m_window->onUpdate();
		}
	}

	bool Application::onWindowClose(WindowCloseEvent& event){
		m_running = false;
		return true;
	}

	bool Application::onWindowResize(WindowResizeEvent& event) {
		return true;
	}
}
