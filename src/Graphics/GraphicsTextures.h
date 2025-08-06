#pragma once

#include "Core.h"
#include "GraphicsType.h"

namespace flaw {
	class Texture {
	public:
		virtual ~Texture() = default;

		virtual ShaderResourceView GetShaderResourceView() const = 0;
		virtual UnorderedAccessView GetUnorderedAccessView() const = 0;
		virtual RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const = 0;
		virtual DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual PixelFormat GetPixelFormat() const = 0;
		virtual UsageFlag GetUsage() const = 0;
		virtual uint32_t GetBindFlags() const = 0;
		virtual uint32_t GetAccessFlags() const = 0;
		virtual uint32_t GetSampleCount() const = 0;
	};

	class Texture2D : public Texture {
	public:
		struct Descriptor {
			const uint8_t* data = nullptr;
			PixelFormat format = PixelFormat::UNDEFINED;
			uint32_t width = 0, height = 0;
			UsageFlag usage = UsageFlag::Static;
			uint32_t access = 0;
			uint32_t bindFlags = 0;
			uint32_t mipLevels = 1;
			uint32_t sampleCount = 1;
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
			bool fromMemory = false;
			std::vector<Ref<Texture2D>> textures;
			const uint8_t* data;
			PixelFormat format;
			uint32_t width, height;
			UsageFlag usage;
			uint32_t access;
			uint32_t bindFlags;
			uint32_t arraySize;
		};

		Texture2DArray() = default;
		virtual ~Texture2DArray() = default;

		virtual void FetchAll(void* outData) const = 0;

		virtual void CopyTo(Ref<Texture2DArray>& target) const = 0;

		virtual uint32_t GetArraySize() const = 0;
	};

	class TextureCube : public Texture {
	public:
		enum class Layout {
			Horizontal,
			HorizontalCross,
		};

		struct Descriptor {
			Layout layout = Layout::Horizontal;
			const uint8_t* data = nullptr;
			PixelFormat format = PixelFormat::UNDEFINED;
			uint32_t width = 0, height = 0;
			UsageFlag usage = UsageFlag::Static;
			uint32_t access = 0;
			uint32_t bindFlags = 0;
			uint32_t mipLevels = 1;
			uint32_t sampleCount = 1;
		};

		TextureCube() = default;
		virtual ~TextureCube() = default;
	};
}

