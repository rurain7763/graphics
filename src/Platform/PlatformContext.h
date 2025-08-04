#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN
namespace vk {
	class Instance;
	class SurfaceKHR;
}
#endif

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <codecvt>
#endif

namespace flaw {
	class EventDispatcher;

	enum class WindowSizeState {
		Normal,
		Maximized,
		Minimized
	};

	class FAPI PlatformContext {
	public:
		PlatformContext(const char* title, int32_t width, int32_t height, EventDispatcher& evnDispatcher);
		~PlatformContext();

		bool PollEvents();
		void GetFrameBufferSize(int32_t& width, int32_t& height);

		void SetTitle(const char* title);

		int32_t GetX() const;
		int32_t GetY() const;
		int32_t GetWidth() const;
		int32_t GetHeight() const;

	#ifdef _WIN32
		void SetUserWndProc(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& userWndProc);
		
		HWND GetWindowHandle();
		
		friend LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	#endif
		
	#ifdef SUPPORT_VULKAN 
		void GetVkRequiredExtensions(std::vector<const char*>& extensions) const;
		void GetVkSurface(vk::Instance& instance, vk::SurfaceKHR& surface) const;
	#endif

	private:
		void CalculateFrameBufferSize();

	private:
		void* _internalData;
	};
}