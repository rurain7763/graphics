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
			
			internalData->_eventDispatcher->Dispatch<WindowResizeEvent>(width, height, internalData->frameBufferWidth, internalData->frameBufferHeight);
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
}

#endif