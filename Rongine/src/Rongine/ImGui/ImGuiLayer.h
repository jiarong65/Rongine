#pragma once
#include "Rongine/Core/Layer.h"
#include "Rongine/Events/KeyEvent.h"
#include "Rongine/Events/MouseEvent.h"
#include "Rongine/Events/ApplicationEvent.h"

namespace Rongine {
	class RONG_API ImGuiLayer :public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void onAttach() override;
		virtual void onDetach() override;

		virtual void onEvent(Event& e) override;

		// 渲染线程：OpenGL3 NewFrame（构建/更新 Font Atlas）
		void updateRendererFrame();
		// 主线程：pollEvents 之后调用（GLFW 输入/显示尺寸，须在 updateRendererFrame 之后）
		void updatePlatformInput();

		void begin();
		void end();

		void initOpenGLBackent();
		void shutdownOpenGLBackend();
		bool isOpenGLBackendReady() const { return m_openGLBackendInitialized; }

		void blockEvents(bool block) { m_blockEvents = block; }
		bool getBlockEvents() const { return m_blockEvents; }
	private:
		float m_time = 0.0f;
		bool m_blockEvents = false;	
		bool m_openGLBackendInitialized=false;
	};

}
