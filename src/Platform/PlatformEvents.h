#pragma once

#include "Core.h"

#include "Input/InputCodes.h"

namespace flaw {
	// platform independent events
	struct FAPI KeyReleaseEvent {
		KeyCode key;

		KeyReleaseEvent(KeyCode key) : key(key) {}
	};

	struct FAPI KeyPressEvent {
		KeyCode key;

		KeyPressEvent(KeyCode key) : key(key) {}
	};

	struct FAPI MouseMoveEvent {
		double x, y;

		MouseMoveEvent(double x, double y) : x(x), y(y) {}
	};

	struct FAPI MouseReleaseEvent {
		MouseButton button;

		MouseReleaseEvent(MouseButton button) : button(button) {}
	};

	struct FAPI MousePressEvent {
		MouseButton button;

		MousePressEvent(MouseButton button) : button(button) {}
	};

	struct FAPI MouseScrollEvent {
		double xOffset, yOffset;

		MouseScrollEvent(double xOffset, double yOffset) : xOffset(xOffset), yOffset(yOffset) {}
	};

	// window events
	struct WindowResizeEvent {
		int32_t width, height;
		int32_t frameBufferWidth, frameBufferHeight;

		WindowResizeEvent(int32_t width, int32_t height, int32_t frameBufferWidth, int32_t frameBufferHeight)
			: width(width)
			, height(height)
			, frameBufferWidth(frameBufferWidth)
			, frameBufferHeight(frameBufferHeight) 
		{
		}
	};

	struct WindowIconifyEvent {
		bool iconified;

		WindowIconifyEvent(bool iconified) : iconified(iconified) {}
	};

	struct WindowFocusEvent {
		bool focused;
		WindowFocusEvent(bool focused) : focused(focused) {}
	};

	struct WindowMoveEvent {
		int32_t x, y;
		WindowMoveEvent(int32_t x, int32_t y) : x(x), y(y) {}
	};
}