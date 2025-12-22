#pragma once
#include "Core.h"
#include "Rongine/Core/KeyCodes.h"    // <--- 必须添加这行
#include "Rongine/Core/MouseCodes.h"  // <--- 必须添加这行
#include <utility>

namespace Rongine {
	class RONG_API Input
	{
	public:
		static bool isKeyPressed(KeyCode key);
		static bool isMouseButtonPressed(MouseCode button);
		static std::pair<float, float> getMousePosition();
		static float getMouseX();
		static float getMouseY();
	};
}

