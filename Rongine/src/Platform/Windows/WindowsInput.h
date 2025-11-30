#pragma once
#include "Rongine/Input.h"

namespace Rongine {
	class WindowsInput:public Input
	{
	public:
		virtual bool isKeyPressedImpl(int keycode) override;
		virtual bool isMouseButtonPressedImpl(int button) override;
		virtual std::pair<float, float> getMousePositionImpl() override;
		virtual float getMouseXImpl() override;
		virtual float getMouseYImpl() override;
	};

}

