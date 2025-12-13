#pragma once
#include "Core.h"

namespace Rongine {
	class RONG_API Input
	{
	public:
		inline static bool isKeyPressed(int keycode) { return s_instance->isKeyPressedImpl(keycode); }
		inline static bool isMouseButtonPressed(int button) { return s_instance->isMouseButtonPressedImpl(button); }
		inline static std::pair<float,float> getMousePosition() { return s_instance->getMousePositionImpl(); }
		inline static float getMouseX() { return s_instance->getMouseXImpl(); };
		inline static float getMouseY() { return s_instance->getMouseYImpl(); }
		
	protected:
		virtual bool isKeyPressedImpl(int keycode) = 0;
		virtual bool isMouseButtonPressedImpl(int button) = 0;
		virtual std::pair<float,float> getMousePositionImpl() = 0;
		virtual float getMouseXImpl() = 0;
		virtual float getMouseYImpl() = 0;
		Input() = default;
		virtual ~Input() = default;
	private:
		static Input* s_instance;
	};
}

