#include "Rongine/Core/Window.h"
#include "Rongpch.h"
#include "Platform/Windows/WindowsWindow.h"

namespace Rongine {

	Window* Window::create(const WindowProps& props)
	{
		#ifdef RONG_PLATFORM_WINDOWS
			return new WindowsWindow(props);
		#else
			#error RONGINE ONLY SUPPORT WINDOWS
		#endif
	}
}