#pragma once

#include "Core.h"
#include "Rongine/Window.h"
#include "Rongine/Events/Event.h"
#include "Rongine/Events/ApplicationEvent.h"
#include "Rongine/Events/KeyEvent.h"
#include "Rongine/Events/MouseEvent.h"
#include "Rongine/LayerStack.h"
#include "Rongine/ImGui/ImGuiLayer.h"
#include "Rongine/Renderer/Shader.h"
#include "Rongine/Renderer/Buffer.h"
#include "Rongine/Renderer/VertexArray.h"

namespace Rongine {
	class RONG_API  Application
	{
	public:
		virtual ~Application();
		void run();

		void onEvent(Event& event);

		void pushLayer(Layer* layer);
		void pushOverLayer(Layer* layer);

		inline Window& getWindow() { return *m_window; }
		inline static Application& get() { return *s_instance; }
	protected:
		Application();
	private:
		bool onWindowResize(WindowResizeEvent& event);
		bool onWindowClose(WindowCloseEvent& event);
	private:
		static Application* s_instance;
		std::unique_ptr<Window> m_window;
		std::shared_ptr<Shader> m_shader;
		std::shared_ptr<VertexArray> m_vertexArray;
		bool m_running = true;
		LayerStack m_layerStack;
		ImGuiLayer* m_imguiLayer;

		std::shared_ptr<Shader> m_blueShader;
		std::shared_ptr<VertexArray> m_squareVA;
	};

	Application* createApplication();
}


