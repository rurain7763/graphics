#include "pch.h"

#ifdef __APPLE__

#include "Platform/PlatformContext.h"
#include "Event/EventDispatcher.h"
#include "Platform/PlatformEvents.h"
#include "Input/InputCodes.h"
#include "Log/Log.h"

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace flaw {
	struct PlatformContextInternalData {
		EventDispatcher* _eventDispatcher;

		GLFWwindow* window;
		int32_t x, y, width, height;
		int32_t frameBufferWidth, frameBufferHeight;

		WindowSizeState windowSizeState;
		std::chrono::steady_clock::time_point lastResizeTime;
	};

	static void GLFWErrorCallback(int error, const char* description) {
		Log::Error("GLFW Error: %d - %s", error, description);
	}

	static KeyCode ConvertToKeyCode(int key) {
		switch (key) {
			case GLFW_KEY_A: return KeyCode::A;
			case GLFW_KEY_B: return KeyCode::B;
			case GLFW_KEY_C: return KeyCode::C;
			case GLFW_KEY_D: return KeyCode::D;
			case GLFW_KEY_E: return KeyCode::E;
			case GLFW_KEY_F: return KeyCode::F;
			case GLFW_KEY_G: return KeyCode::G;
			case GLFW_KEY_H: return KeyCode::H;
			case GLFW_KEY_I: return KeyCode::I;
			case GLFW_KEY_J: return KeyCode::J;
			case GLFW_KEY_K: return KeyCode::K;
			case GLFW_KEY_L: return KeyCode::L;
			case GLFW_KEY_M: return KeyCode::M;
			case GLFW_KEY_N: return KeyCode::N;
			case GLFW_KEY_O: return KeyCode::O;
			case GLFW_KEY_P: return KeyCode::P;
			case GLFW_KEY_Q: return KeyCode::Q;
			case GLFW_KEY_R: return KeyCode::R;
			case GLFW_KEY_S: return KeyCode::S;
			case GLFW_KEY_T: return KeyCode::T;
			case GLFW_KEY_U: return KeyCode::U;
			case GLFW_KEY_V: return KeyCode::V;
			case GLFW_KEY_W: return KeyCode::W;
			case GLFW_KEY_X: return KeyCode::X;
			case GLFW_KEY_Y: return KeyCode::Y;
			case GLFW_KEY_Z: return KeyCode::Z;
			case GLFW_KEY_SPACE: return KeyCode::Space;
			case GLFW_KEY_ESCAPE: return KeyCode::ESCAPE;
			case GLFW_KEY_LEFT_SHIFT: return KeyCode::LSHIFT;
			case GLFW_KEY_LEFT_CONTROL: return KeyCode::LCtrl;
			case GLFW_KEY_LEFT_ALT: return KeyCode::LALT;
			case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RSHIFT;
			case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RCTRL;
			case GLFW_KEY_RIGHT_ALT: return KeyCode::RALT;
			case GLFW_KEY_LEFT: return KeyCode::Left;
			case GLFW_KEY_RIGHT: return KeyCode::Right;
			case GLFW_KEY_UP: return KeyCode::Up;
			case GLFW_KEY_DOWN: return KeyCode::Down;
			case GLFW_KEY_0: return KeyCode::NUM_0;
			case GLFW_KEY_1: return KeyCode::Num1;
			case GLFW_KEY_2: return KeyCode::Num2;
			case GLFW_KEY_3: return KeyCode::Num3;
			case GLFW_KEY_4: return KeyCode::NUM_4;
			case GLFW_KEY_5: return KeyCode::NUM_5;
			case GLFW_KEY_6: return KeyCode::NUM_6;
			case GLFW_KEY_7: return KeyCode::NUM_7;
			case GLFW_KEY_8: return KeyCode::NUM_8;
			case GLFW_KEY_9: return KeyCode::NUM_9;
			default: return KeyCode::Count;
		}
	}

	static MouseButton ConvertToMouseButton(int button) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
			case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
			case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
			default: return MouseButton::Count;
		}
	}

	PlatformContext::PlatformContext(const char* title, int32_t width, int32_t height, EventDispatcher& evnDispatcher) {
		if (!glfwInit()) {
			Log::Error("Failed to initialize GLFW.");
			return;
		}

		auto* internalData = new PlatformContextInternalData();
		internalData->_eventDispatcher = &evnDispatcher;

		glfwSetErrorCallback(GLFWErrorCallback);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		internalData->window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (!internalData->window) {
			throw std::runtime_error("Failed to create GLFW window.");
		}

		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
		
		internalData->x = (mode->width - width) / 2;
		internalData->y = (mode->height - height) / 2;
		internalData->width = width;
		internalData->height = height;
		internalData->windowSizeState = WindowSizeState::Normal;

		_internalData = internalData;
		
		Log::Info("PlatformContext created successfully.");

		glfwSetWindowPos(internalData->window, internalData->x, internalData->y);
		CalculateFrameBufferSize();

		glfwSetWindowUserPointer(internalData->window, internalData);
		
		glfwSetWindowSizeCallback(internalData->window, [](GLFWwindow* window, int width, int height) {
			auto* internalData = static_cast<PlatformContextInternalData*>(glfwGetWindowUserPointer(window));
			internalData->width = width;
			internalData->height = height;
			glfwGetFramebufferSize(window, &internalData->frameBufferWidth, &internalData->frameBufferHeight);
			
			internalData->lastResizeTime = std::chrono::steady_clock::now();
		});

		glfwSetWindowIconifyCallback(internalData->window, [](GLFWwindow* window, int iconified) {
			auto* internalData = static_cast<PlatformContextInternalData*>(glfwGetWindowUserPointer(window));
			internalData->windowSizeState = iconified ? WindowSizeState::Minimized : WindowSizeState::Normal;
			internalData->_eventDispatcher->Dispatch<WindowIconifyEvent>(iconified);
		});

		glfwSetWindowMaximizeCallback(internalData->window, [](GLFWwindow* window, int maximized) {
			auto* internalData = static_cast<PlatformContextInternalData*>(glfwGetWindowUserPointer(window));
			internalData->windowSizeState = maximized ? WindowSizeState::Maximized : WindowSizeState::Normal;
			internalData->_eventDispatcher->Dispatch<WindowResizeEvent>(internalData->width, internalData->height, internalData->frameBufferWidth, internalData->frameBufferHeight);
		});

		glfwSetKeyCallback(internalData->window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			auto* internalData = static_cast<PlatformContextInternalData*>(glfwGetWindowUserPointer(window));

			KeyCode keyCode = ConvertToKeyCode(key);
			if (action == GLFW_PRESS) {
				internalData->_eventDispatcher->Dispatch<KeyPressEvent>(keyCode);
			} else if (action == GLFW_RELEASE) {
				internalData->_eventDispatcher->Dispatch<KeyReleaseEvent>(keyCode);
			}
		});

		glfwSetMouseButtonCallback(internalData->window, [](GLFWwindow* window, int button, int action, int mods) {
			auto* internalData = static_cast<PlatformContextInternalData*>(glfwGetWindowUserPointer(window));
			
			MouseButton mouseButton = ConvertToMouseButton(button);
			if (action == GLFW_PRESS) {
				internalData->_eventDispatcher->Dispatch<MousePressEvent>(mouseButton);
			} else if (action == GLFW_RELEASE) {
				internalData->_eventDispatcher->Dispatch<MouseReleaseEvent>(mouseButton);
			}
		});

		glfwSetCursorPosCallback(internalData->window, [](GLFWwindow* window, double xpos, double ypos) {
			auto* internalData = static_cast<PlatformContextInternalData*>(glfwGetWindowUserPointer(window));
			internalData->_eventDispatcher->Dispatch<MouseMoveEvent>(xpos, ypos);
		});

		glfwSetScrollCallback(internalData->window, [](GLFWwindow* window, double xoffset, double yoffset) {
			auto* internalData = static_cast<PlatformContextInternalData*>(glfwGetWindowUserPointer(window));
			internalData->_eventDispatcher->Dispatch<MouseScrollEvent>(xoffset, yoffset);
		});
	}

	PlatformContext::~PlatformContext() {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);

		glfwDestroyWindow(internalData->window);
		glfwTerminate();

		delete internalData;
	}

	void PlatformContext::CalculateFrameBufferSize() {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);

		glfwGetFramebufferSize(internalData->window, &internalData->frameBufferWidth, &internalData->frameBufferHeight);
	}

	bool PlatformContext::PollEvents() {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);

		glfwPollEvents();

		if (internalData->lastResizeTime.time_since_epoch().count() > 0) {
			if (std::chrono::steady_clock::now() - internalData->lastResizeTime > std::chrono::milliseconds(10)) {
				internalData->_eventDispatcher->Dispatch<WindowResizeEvent>(internalData->width, internalData->height, internalData->frameBufferWidth, internalData->frameBufferHeight);
				internalData->lastResizeTime = std::chrono::steady_clock::time_point();
			}
		}

		return !glfwWindowShouldClose(internalData->window);
	}

	void PlatformContext::GetFrameBufferSize(int32_t& width, int32_t& height) {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		width = internalData->frameBufferWidth;
		height = internalData->frameBufferHeight;
	}

	void PlatformContext::SetTitle(const char* title) {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		glfwSetWindowTitle(internalData->window, title);
	}

	WindowSizeState PlatformContext::GetWindowSizeState() const {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		return internalData->windowSizeState;
	}

	#ifdef SUPPORT_VULKAN
	void PlatformContext::GetVkRequiredExtensions(std::vector<const char*>& extensions) const {
		uint32_t count;
		const char** exts = glfwGetRequiredInstanceExtensions(&count);
		extensions.insert(extensions.end(), exts, exts + count);
		extensions.push_back("VK_KHR_portability_enumeration");
	}

	void PlatformContext::GetVkSurface(vk::Instance& instance, vk::SurfaceKHR& surface) const {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);

		VkSurfaceKHR rawSurface;
		if (glfwCreateWindowSurface(instance, internalData->window, nullptr, &rawSurface) == VK_SUCCESS) {
			surface = rawSurface; 
			return;
		}

		throw std::runtime_error("Failed to create Vulkan surface.");
	}
	#endif
}

#endif