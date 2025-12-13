#pragma once

#include <sstream>
#include <functional>
#include "Rongine/Events/Event.h"

namespace Rongine {

	struct WindowProps {
		
		uint32_t m_width;
		uint32_t m_height;
		std::string m_title;

		WindowProps(const std::string title = "Rongine",
			uint32_t  w=1600, 
			uint32_t  h=900 )
			: m_title(title),m_width(w), m_height(h) {}
	};

	class Window {
	public:
		using eventCallBackFn = std::function<void (Event&)>;
		virtual ~Window() = default;

		virtual void onUpdate() = 0;
		virtual uint32_t getWidth() const = 0;
		virtual uint32_t getHeight() const = 0;

		virtual void setEventCallBack(const eventCallBackFn& fn) = 0;
		virtual void setVSync(bool enabled) = 0;
		virtual bool isVSync() const = 0;

		virtual void* getNativeWindow() const = 0;

		static Window* create(const WindowProps& props = WindowProps());

	};

}