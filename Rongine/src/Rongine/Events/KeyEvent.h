#pragma once

#include "Rongine/Events/Event.h"
#include "Rongine/Core/KeyCodes.h"

namespace Rongine {

	class KeyEvent : public Event
	{
	public:
		KeyCode getKeyCode() const { return m_keyCode; }
		EVENT_CLASS_CATEGORY(EventCategoryInput| EventCategoryKeyboard)

	protected:
		KeyEvent(const KeyCode KeyCode)
			:m_keyCode(KeyCode) {};

		KeyCode m_keyCode;
	};

	//KeyPressedEvent、KeyReleasedEvent、KeyTypedEvent

	class KeyPressedEvent :public KeyEvent
	{
	public:
		KeyPressedEvent(const KeyCode keyCode,const bool isRepeat)
			:KeyEvent(keyCode),m_isRepeat(isRepeat){};
		bool isRepeated() { return m_isRepeat; };

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_keyCode << " (repeat = " << m_isRepeat << ")";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		bool m_isRepeat;
	};

	class KeyReleasedEvent :public KeyEvent
	{
	public:
		KeyReleasedEvent(const KeyCode keyCode)
			:KeyEvent(keyCode) {};

		std::string toString()
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_keyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent :public KeyEvent
	{
	public:
		KeyTypedEvent(const KeyCode keyCode)
			:KeyEvent(keyCode) {};

		std::string toString()
		{
			std::stringstream ss;
			ss << "KeyTypedEvent:" << m_keyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};
}