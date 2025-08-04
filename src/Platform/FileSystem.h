#pragma once

#include "Core.h"

#include <vector>

namespace flaw {
	class FileSystem {
	public:
		static bool MakeFile(const char* path, const int8_t* data = nullptr, uint64_t size = 0);
		static void DestroyFile(const char* path);

		static bool WriteFile(const char* path, const int8_t* data, uint64_t size);
		static bool ReadFile(const char* path, std::vector<int8_t>& out);

		static uint64_t FileIndex(const char* path);

		static std::string GetUniqueFilePath(const char* expectedPath);
		static std::string GetUniqueFolderPath(const char* expectedPath);
	};
}