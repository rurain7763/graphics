#pragma once

#include <string>

namespace flaw{
	class PlatformContext;

	class FileDialogs {
	public:
		static std::string OpenFile(PlatformContext& context, const char* filter);
		static std::string SaveFile(PlatformContext& context, const char* filter);
	};
}