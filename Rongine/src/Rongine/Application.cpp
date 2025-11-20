#include "Rongpch.h"
#include "Application.h"
#include "ApplicationEvent.h"
#include "Log.h"

namespace Rongine {
	Application::Application() {

	}

	Application::~Application() {

	}

	void Application::run() {
		WindowResizeEvent e(1280,720);
		RONG_CLIENT_TRACE( e.toString());

		while (true);
	}
}
