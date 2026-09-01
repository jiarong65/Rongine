module;
#include <cstdint>
#include <string>
#include <sstream>
#include <ostream>

export module Rongine.Events;

import Rongine.Core;

#define EVENT_CLASS_TYPE(type) static EventType getStaticType(){return EventType::type;}\
							   virtual EventType getEventType()const override {return getStaticType();}\
							   virtual const char* getName() const override{return #type;}

#define EVENT_CLASS_CATEGORY(category) virtual int getCategoryFlags()const override {return category;}

export namespace Rongine {

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMove,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication	= 1 << 0,
		EventCategoryInput			= 1 << 1,
		EventCategoryKeyboard		= 1 << 2,
		EventCategoryMouse			= 1 << 3,
		EventCategoryMouseButton	= 1 << 4
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		bool handled = false;
		virtual EventType getEventType() const = 0;
		virtual const char* getName() const = 0;
		virtual int getCategoryFlags() const = 0;
		virtual std::string toString() const {
			return getName();
		}

		bool isInCateGory(EventCategory category)
		{
			return getCategoryFlags() & category;
		}
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event)
			:m_event(event)
		{
		}

		template<class T, class F>
		bool dispatch(const F& func)
		{
			if (m_event.getEventType() == T::getStaticType())
			{
				m_event.handled |= func(static_cast<T&>(m_event));
				return true;
			}
			return false;
		}

	private:
		Event& m_event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.toString();
	}

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(unsigned int m_width, unsigned int m_height)
			:m_width(m_width), m_height(m_height)
		{
		}

		unsigned int getWidth() const { return m_width; }
		unsigned int getHeight() const { return m_height; }

		std::string toString() const override {
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_width << ", " << m_height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		unsigned int m_width, m_height;
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppTickEvent : public Event
	{
	public:
		AppTickEvent() = default;

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent() = default;

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() = default;

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class KeyEvent : public Event
	{
	public:
		KeyCode getKeyCode() const { return m_keyCode; }
		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)

	protected:
		KeyEvent(const KeyCode KeyCode)
			:m_keyCode(KeyCode) {};

		KeyCode m_keyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(const KeyCode keyCode, const bool isRepeat)
			:KeyEvent(keyCode), m_isRepeat(isRepeat) {};
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

	class KeyReleasedEvent : public KeyEvent
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

	class KeyTypedEvent : public KeyEvent
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

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(const float x, const float y)
			:m_mouseX(x), m_mouseY(y) {};

		float getX() { return m_mouseX; }
		float getY() { return m_mouseY; }

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent:" << m_mouseX << " ," << m_mouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
	private:
		float m_mouseX, m_mouseY;
	};

	class MouseScrolledEvent : public Event
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

	class MouseButtonEvent : public Event
	{
	public:
		MouseCode getMouseButton() { return m_mouseCode; }

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton)

	protected:
		MouseButtonEvent(const MouseCode button)
			:m_mouseCode(button) {};
		MouseCode m_mouseCode;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(const MouseCode button)
			:MouseButtonEvent(button) {};

		std::string toString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_mouseCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)

	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(const MouseCode button)
			:MouseButtonEvent(button) {};

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}
