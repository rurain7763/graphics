#include "pch.h"

#ifdef _WIN32

#include "Platform/FileDialogs.h"
#include "Platform/Windows/WindowsContext.h"

#include <Windows.h>
#include <commdlg.h>

namespace flaw {
	std::string FileDialogs::OpenFile(PlatformContext& context, const char* filter) {
		WindowsContext& winContext = static_cast<WindowsContext&>(context);

		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = winContext.GetWindowHandle();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 2;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetOpenFileNameA(&ofn) == TRUE) {
			return ofn.lpstrFile;
		}

		return "";
	}

	std::string FileDialogs::SaveFile(PlatformContext& context, const char* filter) {
		WindowsContext& winContext = static_cast<WindowsContext&>(context);

		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = winContext.GetWindowHandle();
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		if (GetSaveFileNameA(&ofn) == TRUE) {
			return ofn.lpstrFile;
		}

		return "";
	}
}

#endif
