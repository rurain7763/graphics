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