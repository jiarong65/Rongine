#pragma once

#include "Rongine/Events/Event.h"
#include "Rongine/Core/MouseCodes.h"

namespace Rongine {

	class MouseMovedEvent :public Event
	{
	public:
		MouseMovedEvent(const float x,const float y)
			:m_mouseX(x),m_mouseY(y){};

		float getX() { return m_mouseX; }
		float getY() { return m_mouseY; }

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent:" << m_mouseX << " ," << m_mouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryInput| EventCategoryMouse)
	private:
		float m_mouseX, m_mouseY;
	};

	class MouseScrolledEvent :public Event
	{
	public:
		MouseScrolledEvent(const float x, const float y)
			:m_xOffset(x), m_yOffset(y) {};

		float getXOffset() { return m_xOffset; }
		float getYOffset() { return m_yOffset; }

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << m_xOffset << " ," << m_yOffset;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
	
	private:
		float m_xOffset, m_yOffset;
	};

	class MouseButtonEvent :public Event
	{
	public:
		MouseCode getMouseButton() { return m_mouseCode; }

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton)

	protected:
		MouseButtonEvent(const MouseCode button)
			:m_mouseCode(button) {};
		MouseCode m_mouseCode;
	};

	class MouseButtonPressedEvent :public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(const MouseCode button)
			:MouseButtonEvent(button) {};

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << m_mouseCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)

	};

	class MouseButtonReleasedEvent :public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(const MouseCode button)
			:MouseButtonEvent(button) {};

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << m_mouseCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}