#include "pch.h"
#include "Input.h"

namespace flaw {
	static std::bitset<static_cast<size_t>(KeyCode::Count)> keyState;
	static std::bitset<static_cast<size_t>(KeyCode::Count)> currDownKeyState;
	static std::bitset<static_cast<size_t>(KeyCode::Count)> currUpKeyState;

	static double mouseX = 0;
	static double mouseY = 0;
	static double mouseScrollX = 0;
	static double mouseScrollY = 0;

	static std::bitset<static_cast<size_t>(MouseButton::Count)> mouseButtonState;
	static std::bitset<static_cast<size_t>(MouseButton::Count)> currDownMouseButtonState;
	static std::bitset<static_cast<size_t>(MouseButton::Count)> currUpMouseButtonState;

	void Input::Update() {
		currDownKeyState.reset();
		currUpKeyState.reset();

		currDownMouseButtonState.reset();
		currUpMouseButtonState.reset();

		mouseScrollX = 0;
		mouseScrollY = 0;
	}

	void Input::Reset() {
		keyState.reset();
		currDownKeyState.reset();
		currUpKeyState.reset();

		mouseX = 0;
		mouseY = 0;
		mouseScrollX = 0;
		mouseScrollY = 0;

		mouseButtonState.reset();
		currDownMouseButtonState.reset();
		currUpMouseButtonState.reset();
	}

	void Input::OnKeyPress(KeyCode key) {
		keyState.set(static_cast<size_t>(key), true);
		currDownKeyState.set(static_cast<size_t>(key), true);
	}

	void Input::OnKeyRelease(KeyCode key) {
		keyState.set(static_cast<size_t>(key), false);
		currUpKeyState.set(static_cast<size_t>(key), true);
	}

	void Input::OnMouseMove(double x, double y) {
		mouseX = x;
		mouseY = y;
	}

	void Input::OnMousePress(MouseButton button) {
		mouseButtonState.set(static_cast<size_t>(button), true);
		currDownMouseButtonState.set(static_cast<size_t>(button), true);
	}

	void Input::OnMouseRelease(MouseButton button) {
		mouseButtonState.set(static_cast<size_t>(button), false);
		currUpMouseButtonState.set(static_cast<size_t>(button), true);
	}

	void Input::OnMouseScroll(double xOffset, double yOffset) {
		mouseScrollX = xOffset;
		mouseScrollY = yOffset;
	}

	bool Input::GetKeyDown(KeyCode key) {
		return currDownKeyState.test(static_cast<size_t>(key));
	}

	bool Input::GetKeyUp(KeyCode key) {
		return currUpKeyState.test(static_cast<size_t>(key));
	}

	bool Input::GetKey(KeyCode key) {
		return keyState.test(static_cast<size_t>(key));
	}

	double Input::GetMouseX() { return mouseX; }
	double Input::GetMouseY() { return mouseY; }
	double Input::GetMouseScrollX() { return mouseScrollX; }
	double Input::GetMouseScrollY() { return mouseScrollY; }

	bool Input::GetMouseButtonDown(MouseButton button) {
		return currDownMouseButtonState.test(static_cast<size_t>(button));
	}

	bool Input::GetMouseButtonUp(MouseButton button) {
		return currUpMouseButtonState.test(static_cast<size_t>(button));
	}

	bool Input::GetMouseButton(MouseButton button) {
		return mouseButtonState.test(static_cast<size_t>(button));
	}
}