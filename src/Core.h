#pragma once

#include <stdint.h>
#include <memory>
#include <iostream>
#include <string>
#include <locale> 
#include <codecvt>

#ifdef _WIN32

#define SUPPORT_VULKAN

#ifdef FL_DYNAMIC_LIB
	#ifdef FL_DLL_EXPORT
		#define FAPI __declspec(dllexport)
	#else
		#define FAPI __declspec(dllimport)
	#endif
#else
	#define FAPI
#endif

#define vsprintf(buff, size, format, args) vsprintf_s(buff, size, format, args)

#elif __APPLE__

#define SUPPORT_VULKAN

#ifdef FL_DYNAMIC_LIB
	#ifdef FL_DLL_EXPORT
		#define FAPI __attribute__((visibility("default")))
	#else
		#define FAPI
	#endif
#else
	#define FAPI
#endif

#define vsprintf(buff, size, format, args) vsnprintf(buff, size, format, args)

#endif

#ifdef FL_ENABLE_ASSERTS
	#define FASSERT(x, ...) { if(!(x)) { std::cerr << "Assertion Failed: " << __VA_ARGS__ << std::endl; __debugbreak(); } }
#else
	#define FASSERT(x, ...)
#endif

namespace flaw {
	inline uint64_t PID(void* ptr) noexcept {
		return reinterpret_cast<uint64_t>(ptr);
	}

	template <typename T>
	inline std::string_view TypeName() {
		std::string_view name = typeid(T).name();
		for (int32_t i = name.size() - 1; i >= 0; --i) {
			if (name[i] == ' ' || name[i] == ':') {
				return name.substr(i + 1);
			}
		}
		return name;
	}

	inline std::string Utf16ToUtf8(const std::wstring& utf16) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.to_bytes(utf16);
	}

	inline std::wstring Utf8ToUtf16(const std::string& utf8) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(utf8);
	}

	template <typename T>
	using Ref = std::shared_ptr<T>;

	template <typename T, typename... Args>
	constexpr Ref<T> CreateRef(Args&&... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	using Scope = std::unique_ptr<T>;

	template <typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
}
