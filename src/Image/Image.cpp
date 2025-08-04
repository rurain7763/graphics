#include "pch.h"
#include "Image.h"
#include "Log/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define IMATH_HALF_NO_LOOKUP_TABLE
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfArray.h>

namespace flaw {
	Image::Image(const char* filePath, uint32_t desiredChannels) {
		_type = GetImageTypeFromExtension(filePath);

		if (_type == Image::Type::Unknown) {
			Log::Error("Unknown image format : %s", filePath);
			return;
		}

		if (_type == Image::Type::Exr) {
			Imf::RgbaInputFile file(filePath);
			Imath::Box2i dw = file.header().dataWindow();

			_width = dw.max.x - dw.min.x + 1;
			_height = dw.max.y - dw.min.y + 1;

			Imf::Array2D<Imf::Rgba> pixels(_height, _width);

			file.setFrameBuffer(&pixels[0][0] - dw.min.x - dw.min.y * _width, 1, _width);
			file.readPixels(dw.min.y, dw.max.y);

			if (desiredChannels == 0) {
				desiredChannels = 4; // RGBA
			}

			std::vector<float> data(_width * _height * desiredChannels);
			for (int32_t y = 0; y < _height; ++y) {
				for (int32_t x = 0; x < _width; ++x) {
					Imf::Rgba& pixel = pixels[y][x];

					int32_t pixelIndex = (y * _width + x) * desiredChannels;

					if (desiredChannels == 1) {
						data[pixelIndex] = pixel.r; // R
					}
					else if (desiredChannels == 3) {
						data[pixelIndex] = pixel.r; // R
						data[pixelIndex + 1] = pixel.g; // G
						data[pixelIndex + 2] = pixel.b; // B
					}
					else if (desiredChannels == 4) {
						data[pixelIndex] = pixel.r; // R
						data[pixelIndex + 1] = pixel.g; // G
						data[pixelIndex + 2] = pixel.b; // B
						data[pixelIndex + 3] = pixel.a; // A
					}
				}
			}

			_data.resize(_width * _height * desiredChannels * sizeof(float));
			memcpy(_data.data(), data.data(), _data.size());
		}
		else if (_type == Image::Type::Hdr) {
			float* data = stbi_loadf(filePath, &_width, &_height, &_channels, desiredChannels);
			if (data == nullptr) {
				Log::Error("Failed to load HDR image : %s", filePath);
				return;
			}

			if (desiredChannels != 0) {
				_channels = desiredChannels;
			}

			_data.resize(_width * _height * _channels * sizeof(float));
			memcpy(_data.data(), data, _data.size());

			stbi_image_free(data);
		}
		else {
			uint8_t* data = stbi_load(filePath, &_width, &_height, &_channels, desiredChannels);
	
			if (data == nullptr) {
				Log::Error("Failed to load image : %s", filePath);
				return;
			}
	
			if (desiredChannels != 0) {
				_channels = desiredChannels;
			}
	
			_data.resize(_width * _height * _channels);
			memcpy(_data.data(), data, _data.size());
	
			stbi_image_free(data);
		}
	}

	Image::Image(Image::Type type, const char* source, size_t size, uint32_t desiredChannels) {
		_type = type;

		if (_type == Image::Type::Unknown) {
			Log::Error("Unknown image format");
			return;
		}

		if (_type == Image::Type::Exr) {
			// TODO: EXR �޸� �ε� ����
		}
		else if (_type == Image::Type::Hdr) {
			float* data = stbi_loadf_from_memory((const uint8_t*)source, size, &_width, &_height, &_channels, desiredChannels);
			if (data == nullptr) {
				Log::Error("Failed to load HDR image from memory");
				return;
			}

			if (desiredChannels != 0) {
				_channels = desiredChannels;
			}

			_data.resize(_width * _height * _channels * sizeof(float));
			memcpy(_data.data(), data, _data.size());

			stbi_image_free(data);
		}
		else {
			uint8_t* data = stbi_load_from_memory((const uint8_t*)source, size, &_width, &_height, &_channels, desiredChannels);

			if (data == nullptr) {
				Log::Error("Failed to load image");
				return;
			}

			if (desiredChannels != 0) {
				_channels = desiredChannels;
			}

			_data.resize(_width * _height * _channels);
			memcpy(_data.data(), data, _data.size());

			stbi_image_free(data);
		}
	}

	void Image::SaveToFile(const char* filePath) const {
		if (_data.empty()) {
			Log::Error("Image data is empty");
			return;
		}

		SaveToFile(filePath, _data.data(), _width, _height, _type, _channels);
	}

	Image::Type Image::GetImageTypeFromExtension(const char* filePath) {
		std::filesystem::path path(filePath);

		if (path.extension() == ".png") {
			return Image::Type::Png;
		}
		else if (path.extension() == ".jpg") {
			return Image::Type::Jpg;
		}
		else if (path.extension() == ".bmp") {
			return Image::Type::Bmp;
		}
		else if (path.extension() == ".tga") {
			return Image::Type::Tga;
		}
		else if (path.extension() == ".dds") {
			return Image::Type::Dds;
		}
		else if (path.extension() == ".exr") {
			return Image::Type::Exr;
		}
		else if (path.extension() == ".hdr") {
			return Image::Type::Hdr;
		}
		else {
			return Image::Type::Unknown;
		}
	}

	void Image::SaveToFile(const char* filePath, const void* data, int32_t width, int32_t height, Image::Type type, int32_t channels) {
		switch (type) {
		case Image::Type::Png:
			stbi_write_png(filePath, width, height, channels, data, width * channels);
			break;
		case Image::Type::Jpg:
			stbi_write_jpg(filePath, width, height, channels, data, 100);
			break;
		case Image::Type::Bmp:
			stbi_write_bmp(filePath, width, height, channels, data);
			break;
		case Image::Type::Tga:
			stbi_write_tga(filePath, width, height, channels, data);
			break;
		case Image::Type::Exr:
			// TODO: EXR ���� ����
			break;
		case Image::Type::Hdr:
			stbi_write_hdr(filePath, width, height, channels, (const float*)data);
			break;
		case Image::Type::Dds:
			Log::Error("DDS format is not supported for saving");
			break;
		default:
			Log::Error("Unknown image format");
			break;
		}
	}
}
