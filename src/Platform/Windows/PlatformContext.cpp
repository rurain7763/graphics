#include "pch.h"

#ifdef _WIN32

#include "Platform/PlatformContext.h"
#include "Event/EventDispatcher.h"
#include "Platform/PlatformEvents.h"
#include "Input/InputCodes.h"
#include "Log/Log.h"

#ifdef SUPPORT_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#endif

namespace flaw {
	struct PlatformContextInternalData {
		EventDispatcher* eventDispatcher;

		HWND window;
		int32_t x, y;
		int32_t width, height;
		int32_t frameBufferWidth, frameBufferHeight;
		WindowSizeState windowSizeState;

		std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> userWndProc;
	};

	KeyCode TranslateKeyCode(WPARAM wParam) {
		switch (wParam) {
		case VK_ESCAPE: return KeyCode::ESCAPE;
		case 'A': return KeyCode::A;
		case 'B': return KeyCode::B;
		case 'C': return KeyCode::C;
		case 'D': return KeyCode::D;
		case 'E': return KeyCode::E;
		case 'F': return KeyCode::F;
		case 'G': return KeyCode::G;
		case 'H': return KeyCode::H;
		case 'I': return KeyCode::I;
		case 'J': return KeyCode::J;
		case 'K': return KeyCode::K;
		case 'L': return KeyCode::L;
		case 'M': return KeyCode::M;
		case 'N': return KeyCode::N;
		case 'O': return KeyCode::O;
		case 'P': return KeyCode::P;
		case 'Q': return KeyCode::Q;
		case 'R': return KeyCode::R;
		case 'S': return KeyCode::S;
		case 'T': return KeyCode::T;
		case 'U': return KeyCode::U;
		case 'V': return KeyCode::V;
		case 'W': return KeyCode::W;
		case 'X': return KeyCode::X;
		case 'Y': return KeyCode::Y;
		case 'Z': return KeyCode::Z;
		case VK_SPACE: return KeyCode::Space;
		case VK_SHIFT: return KeyCode::LSHIFT;
		case VK_CONTROL: return KeyCode::LCtrl;
		case VK_MENU: return KeyCode::LALT;
		case VK_LEFT: return KeyCode::Left;
		case VK_RIGHT: return KeyCode::Right;
		case VK_UP: return KeyCode::Up;
		case VK_DOWN: return KeyCode::Down;
		case '0': return KeyCode::NUM_0;
		case '1': return KeyCode::Num1;
		case '2': return KeyCode::Num2;
		case '3': return KeyCode::Num3;
		case '4': return KeyCode::NUM_4;
		case '5': return KeyCode::NUM_5;
		case '6': return KeyCode::NUM_6;
		case '7': return KeyCode::NUM_7;
		case '8': return KeyCode::NUM_8;
		case '9': return KeyCode::NUM_9;
		}

		return KeyCode::Count;
	}

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		PlatformContext* context = reinterpret_cast<PlatformContext*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (!context) {
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		auto* internalData = static_cast<PlatformContextInternalData*>(context->_internalData);

		if (internalData->userWndProc && internalData->userWndProc(hWnd, message, wParam, lParam)) {
			return 0;
		}

		switch (message) {
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		case WM_KEYDOWN:
		{
			// repeat key check
			if (lParam & 0x40000000) break;

			KeyCode keyCode = TranslateKeyCode(wParam);
			if (keyCode != KeyCode::Count) {
				internalData->eventDispatcher->Dispatch<KeyPressEvent>(keyCode);
			}
			break;
		}
		case WM_KEYUP:
		{
			KeyCode keyCode = TranslateKeyCode(wParam);
			if (keyCode != KeyCode::Count) {
				internalData->eventDispatcher->Dispatch<KeyReleaseEvent>(keyCode);
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			double x = GET_X_LPARAM(lParam);
			double y = GET_Y_LPARAM(lParam);
			internalData->eventDispatcher->Dispatch<MouseMoveEvent>(x, y);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			internalData->eventDispatcher->Dispatch<MousePressEvent>(MouseButton::Left);
			break;
		}
		case WM_RBUTTONDOWN:
		{
			internalData->eventDispatcher->Dispatch<MousePressEvent>(MouseButton::Right);
			break;
		}
		case WM_MBUTTONDOWN:
		{
			internalData->eventDispatcher->Dispatch<MousePressEvent>(MouseButton::Middle);
			break;
		}
		case WM_LBUTTONUP:
		{
			internalData->eventDispatcher->Dispatch<MouseReleaseEvent>(MouseButton::Left);
			break;
		}
		case WM_RBUTTONUP:
		{
			internalData->eventDispatcher->Dispatch<MouseReleaseEvent>(MouseButton::Right);
			break;
		}
		case WM_MBUTTONUP:
		{
			internalData->eventDispatcher->Dispatch<MouseReleaseEvent>(MouseButton::Middle);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			int16_t xOffset = GET_WHEEL_DELTA_WPARAM(wParam);
			int16_t yOffset = GET_WHEEL_DELTA_WPARAM(wParam);
			internalData->eventDispatcher->Dispatch<MouseScrollEvent>(xOffset / WHEEL_DELTA, yOffset / WHEEL_DELTA);
			break;
		}
		case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED) {
				internalData->windowSizeState = WindowSizeState::Minimized;
				internalData->eventDispatcher->Dispatch<WindowIconifyEvent>(true);
				break;
			}

			RECT rect;
			GetClientRect(hWnd, &rect);
			internalData->width = rect.right - rect.left;
			internalData->height = rect.bottom - rect.top;
			context->CalculateFrameBufferSize();

			if (wParam == SIZE_MAXIMIZED) {
				if (internalData->windowSizeState == WindowSizeState::Minimized) {
					internalData->eventDispatcher->Dispatch<WindowIconifyEvent>(false);
				}

				internalData->windowSizeState = WindowSizeState::Maximized;
				internalData->eventDispatcher->Dispatch<WindowResizeEvent>(internalData->width, internalData->height, internalData->frameBufferWidth, internalData->frameBufferHeight);
			}
			else if (wParam == SIZE_RESTORED) {
				if (internalData->windowSizeState == WindowSizeState::Maximized) {
					internalData->eventDispatcher->Dispatch<WindowResizeEvent>(internalData->width, internalData->height, internalData->frameBufferWidth, internalData->frameBufferHeight);
				}
				else if (internalData->windowSizeState == WindowSizeState::Minimized) {
					internalData->eventDispatcher->Dispatch<WindowIconifyEvent>(false);
				}

				internalData->windowSizeState = WindowSizeState::Normal;
			}
			break;
		}
		case WM_EXITSIZEMOVE:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);

			internalData->width = rect.right - rect.left;
			internalData->height = rect.bottom - rect.top;
			context->CalculateFrameBufferSize();
			internalData->eventDispatcher->Dispatch<WindowResizeEvent>(internalData->width, internalData->height, internalData->frameBufferWidth, internalData->frameBufferHeight);
			break;
		}
		case WM_KILLFOCUS:
		{
			internalData->eventDispatcher->Dispatch<WindowFocusEvent>(false);
			break;
		}
		case WM_SETFOCUS:
		{
			internalData->eventDispatcher->Dispatch<WindowFocusEvent>(true);
			break;
		}
		case WM_MOVE:
		{
			internalData->x = GET_X_LPARAM(lParam);
			internalData->y = GET_Y_LPARAM(lParam);
			internalData->eventDispatcher->Dispatch<WindowMoveEvent>(internalData->x, internalData->y);
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		return 0;
	}

	PlatformContext::PlatformContext(const char* title, int32_t width, int32_t height, EventDispatcher& evnDispatcher) {
		PlatformContextInternalData* internalData = new PlatformContextInternalData();
		internalData->eventDispatcher = &evnDispatcher;
		
		// init hInstance
		HINSTANCE hInstance = GetModuleHandleA(nullptr);

		std::wstring wAppName = L"FlawEngine";
		std::wstring wTitle = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(title);

		// Register class
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
		wcex.hIconSm = (HICON)LoadImage(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
		wcex.hCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = wAppName.c_str();

		RegisterClassEx(&wcex);

		internalData->window = CreateWindow(
			wAppName.c_str(),
			wTitle.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width,
			height,
			nullptr,
			nullptr,
			hInstance,
			nullptr
		);

		SetWindowLongPtr(internalData->window, GWLP_USERDATA, (LONG_PTR)this);

		// Get primary monitor resolution
		HMONITOR hMonitor = MonitorFromWindow(internalData->window, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = { sizeof(MONITORINFO) };
		GetMonitorInfo(hMonitor, &mi);

		// Center window
		int32_t x = (mi.rcMonitor.right - width) / 2;
		int32_t y = (mi.rcMonitor.bottom - height) / 2;

		internalData->x = x;
		internalData->y = y;
		internalData->width = width;
		internalData->height = height;
		internalData->windowSizeState = WindowSizeState::Normal;

		_internalData = internalData;

		// Adjust window size
		RECT rect = { 0, 0, width, height };
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
		SetWindowPos(internalData->window, nullptr, x, y, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_FRAMECHANGED);

		ShowWindow(internalData->window, SW_SHOW);
		UpdateWindow(internalData->window);

		CalculateFrameBufferSize();
	}

	PlatformContext::~PlatformContext() {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);

		DestroyWindow(internalData->window);
		UnregisterClass(L"CLIENT", GetModuleHandleA(nullptr));

		delete internalData;
	}

	void PlatformContext::CalculateFrameBufferSize() {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);

		HDC hdc = GetDC(internalData->window);

		const float defaultDPI = 96.0f;

		// Get the DPI scale factor
		float dpiX = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSX)) / defaultDPI;
		float dpiY = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSY)) / defaultDPI;

		// Calculate framebuffer size
		internalData->frameBufferWidth = static_cast<int32_t>(internalData->width * dpiX);
		internalData->frameBufferHeight = static_cast<int32_t>(internalData->height * dpiY);

		ReleaseDC(internalData->window, hdc);
	}

	bool PlatformContext::PollEvents() {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);

			if (msg.message == WM_QUIT)
				return false;

			DispatchMessage(&msg);
		}

		return true;
	}

	void PlatformContext::SetTitle(const char* title) {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);

		std::wstring wTitle = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(title);
		SetWindowText(internalData->window, wTitle.c_str());
	}

	int32_t PlatformContext::GetX() const {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		return internalData->x;
	}

	int32_t PlatformContext::GetY() const {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		return internalData->y;
	}

	int32_t PlatformContext::GetWidth() const {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		return internalData->width;
	}

	int32_t PlatformContext::GetHeight() const {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		return internalData->height;
	}

	WindowSizeState PlatformContext::GetWindowSizeState() const {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		return internalData->windowSizeState;
	}

	void PlatformContext::GetFrameBufferSize(int32_t& width, int32_t& height) {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		width = internalData->frameBufferWidth;
		height = internalData->frameBufferHeight;
	}

	void PlatformContext::SetUserWndProc(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& userWndProc) {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		internalData->userWndProc = userWndProc;
	}

	HWND PlatformContext::GetWindowHandle() {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		return internalData->window;
	}

#ifdef SUPPORT_VULKAN
	void PlatformContext::GetVkRequiredExtensions(std::vector<const char*>& extensions) const {
		extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	}

	void PlatformContext::GetVkSurface(vk::Instance& instance, vk::SurfaceKHR& surface) const {
		auto* internalData = static_cast<PlatformContextInternalData*>(_internalData);
		
		vk::Win32SurfaceCreateInfoKHR createInfo;
		createInfo.hinstance = GetModuleHandle(nullptr);
		createInfo.hwnd = internalData->window;

		surface = instance.createWin32SurfaceKHR(createInfo);
	}
#endif
}

#endif