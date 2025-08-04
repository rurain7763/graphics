#pragma once

#include "Core.h"

#include <vector>

namespace flaw {
	class Image {
	public:
		enum class Type {
			Png,
			Jpg,
			Bmp,
			Tga,
			Dds,
			Exr,
			Hdr,
			Unknown
		};

		Image() = default;
		Image(const char* filePath, uint32_t desiredChannels = 0);
		Image(Type type, const char* memory, size_t size, uint32_t desiredChannels = 0);

		void SaveToFile(const char* filePath) const;

		Type ImageType() const { return _type; }
		const std::vector<uint8_t>& Data() const { return _data; }
		int32_t Width() const { return _width; }
		int32_t Height() const { return _height; }
		int32_t Channels() const { return _channels; }

		bool IsValid() const { return !_data.empty(); }

		static void SaveToFile(const char* filePath, const void* data, int32_t width, int32_t height, Type type, int32_t channels = 4);
		static Type GetImageTypeFromExtension(const char* filePath);

	private:
		Type _type = Type::Unknown;

		std::vector<uint8_t> _data;
		int32_t _width = 0;
		int32_t _height = 0;
		int32_t _channels = 0;
	};
}
