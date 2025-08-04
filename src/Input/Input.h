#pragma once

#include "Core.h"
#include "InputCodes.h"

namespace flaw {
	class FAPI Input {
	public:
		static void Update();
		static void Reset();

		static void OnKeyPress(KeyCode key);
		static void OnKeyRelease(KeyCode key);

		static void OnMouseMove(double x, double y);
		static void OnMousePress(MouseButton button);
		static void OnMouseRelease(MouseButton button);
		static void OnMouseScroll(double xOffset, double yOffset);

		static bool GetKeyDown(KeyCode key);
		static bool GetKeyUp(KeyCode key);

		static bool GetKey(KeyCode key);

		static double GetMouseX();
		static double GetMouseY();
		static double GetMouseScrollX();
		static double GetMouseScrollY();

		static bool GetMouseButtonDown(MouseButton button);
		static bool GetMouseButtonUp(MouseButton button);
		static bool GetMouseButton(MouseButton button);
	};
}

