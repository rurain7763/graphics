#pragma once

#include "Core.h"
#include "GraphicsType.h"

namespace flaw {
	struct NativeTexture {};
	struct NativeTextureView {};

	class Texture {
	public:
		virtual ~Texture() = default;

		virtual const NativeTexture& GetNativeTexture() const = 0;
		virtual const NativeTextureView& GetNativeTextureView() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual PixelFormat GetPixelFormat() const = 0;
		virtual TextureUsages GetUsages() const = 0;
		virtual uint32_t GetSampleCount() const = 0;
	};

	class Texture2D : public Texture {
	public:
		struct Descriptor {
			const uint8_t* data = nullptr;
			PixelFormat format = PixelFormat::Undefined;
			uint32_t width = 0, height = 0;
			MemoryProperty memProperty = MemoryProperty::Static;
			TextureUsages texUsages = 0;
			uint32_t mipLevels = 1;
			uint32_t sampleCount = 1;
			TextureLayout initialLayout = TextureLayout::Undefined;
		};

		Texture2D() = default;
		virtual ~Texture2D() = default;

		virtual void Fetch(void* outData, uint32_t size) const = 0;

		virtual void CopyTo(Ref<Texture2D>& target) const = 0;
		virtual void CopyToSub(Ref<Texture2D>& target, uint32_t x, uint32_t y, uint32_t width, uint32_t height) const = 0;
	};

	class Texture2DArray : public Texture {
	public:
		struct Descriptor {
			const uint8_t* data = nullptr;
			PixelFormat format = PixelFormat::Undefined;
			uint32_t width = 0, height = 0;
			MemoryProperty memProperty = MemoryProperty::Static;
			TextureUsages texUsages = 0;
			uint32_t arraySize = 0;
			uint32_t mipLevels = 1;
			uint32_t sampleCount = 1;
			TextureLayout initialLayout = TextureLayout::Undefined;
		};

		Texture2DArray() = default;
		virtual ~Texture2DArray() = default;

		virtual void Fetch(void* outData, const uint32_t size) const = 0;

		virtual void CopyTo(Ref<Texture2DArray>& target) const = 0;

		virtual uint32_t GetArraySize() const = 0;
	};

	class TextureCube : public Texture {
	public:
		struct Descriptor {
			const uint8_t* data = nullptr;
			PixelFormat format = PixelFormat::Undefined;
			uint32_t width = 0, height = 0;
			MemoryProperty memProperty = MemoryProperty::Static;
			TextureUsages texUsages = 0;
			uint32_t mipLevels = 1;
			uint32_t sampleCount = 1;
			TextureLayout initialLayout = TextureLayout::Undefined;
		};

		TextureCube() = default;
		virtual ~TextureCube() = default;
	};
}

