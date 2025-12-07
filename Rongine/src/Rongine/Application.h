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
#include "Rongine/Renderer/OrthographicCamera.h"
#include "Rongine/Core/Timestep.h"

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

		bool m_running = true;
		LayerStack m_layerStack;
		ImGuiLayer* m_imguiLayer;

		float m_lastFrameTime = 0.0f;
	};

	Application* createApplication();
}


