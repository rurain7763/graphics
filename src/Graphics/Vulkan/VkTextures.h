#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsTextures.h"

namespace flaw {
	class VkContext;

	struct VkNativeTexture : public NativeTexture {
		vk::Image image;
		vk::DeviceMemory memory;

		static VkNativeTexture Create(VkContext& context, const vk::ImageCreateInfo& imageCreateInfo, vk::MemoryPropertyFlags memoryProperties);
	};

	struct VkNativeTextureView : public NativeTextureView {
		vk::ImageView imageView;
	};

	class VkTexture2D : public Texture2D {
	public:
		VkTexture2D(VkContext& context, const Descriptor& descriptor);
		VkTexture2D(VkContext& context, vk::Image image, uint32_t width, uint32_t height, PixelFormat format, MemoryProperty usage, uint32_t bindFlags, uint32_t sampleCount, uint32_t mipLevels);
		~VkTexture2D();

		void Fetch(void* outData, const uint32_t size) const override;

		void CopyTo(Ref<Texture2D>& target) const override {}
		void CopyToSub(Ref<Texture2D>& target, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) const override {}

		const NativeTexture& GetNativeTexture() const override { return _nativeTexture; }
		const NativeTextureView& GetNativeTextureView() const override { return _nativeTextureView; }

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _texUsages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }

		inline vk::Sampler GetVkSampler() const { return _sampler; }

	private:
		bool PullMemory(vk::CommandBuffer& commandBuffer, const uint8_t* data);
		bool GenerateMipmaps(vk::CommandBuffer& commandBuffer);
		bool TransitionFinalImageLayout(vk::CommandBuffer& commandBuffer, TextureLayout layout);

		bool CreateImageView();
		bool CreateSampler();

	private:
		VkContext& _context;

		bool _isExternalImage = false;

		VkNativeTexture _nativeTexture;
		VkNativeTextureView _nativeTextureView;

		vk::Sampler _sampler;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _texUsages;
		uint32_t _mipLevels;
		uint32_t _sampleCount;

		uint32_t _width;
		uint32_t _height;
	};

	class VkTexture2DArray : public Texture2DArray {
	public:
		VkTexture2DArray(VkContext& context, const Descriptor& descriptor);
		~VkTexture2DArray();

		void Fetch(void* outData, const uint32_t size) const override {}

		void CopyTo(Ref<Texture2DArray>& target) const override {}

		const NativeTexture& GetNativeTexture() const override { return _nativeTexture; }
		const NativeTextureView& GetNativeTextureView() const override { return _nativeTextureView; }

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _texUsages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }
		uint32_t GetArraySize() const override { return _arraySize; }

		inline vk::Sampler GetVkSampler() const { return _sampler; }

	private:
		bool PullMemory(vk::CommandBuffer& commandBuffer, const uint8_t* data);
		bool GenerateMipmaps(vk::CommandBuffer& commandBuffer);
		bool TransitionFinalImageLayout(vk::CommandBuffer& commandBuffer, TextureLayout layout);

		bool CreateImageView();
		bool CreateSampler();

	private:
		VkContext& _context;

		VkNativeTexture _nativeTexture;
		VkNativeTextureView _nativeTextureView;

		vk::Sampler _sampler;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _texUsages;
		uint32_t _arraySize;
		uint32_t _mipLevels;
		uint32_t _sampleCount;

		uint32_t _width;
		uint32_t _height;
	};

	class VkTextureCube : public TextureCube {
	public:
		VkTextureCube(VkContext& context, const Descriptor& descriptor);
		~VkTextureCube();

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _texUsages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }

		const NativeTexture& GetNativeTexture() const override { return _nativeTexture; }
		const NativeTextureView& GetNativeTextureView() const override { return _nativeTextureView; }

		inline vk::Sampler GetVkSampler() const { return _sampler; }

	private:
		bool PullMemory(vk::CommandBuffer& commandBuffer, const uint8_t* data);
		bool GenerateMipmaps(vk::CommandBuffer& commandBuffer);
		bool TransitionFinalImageLayout(vk::CommandBuffer& commandBuffer, TextureLayout layout);

		bool CreateImageView();
		bool CreateSampler();

	private:
		VkContext& _context;

		VkNativeTexture _nativeTexture;
		VkNativeTextureView _nativeTextureView;

		vk::Sampler _sampler;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _texUsages;
		uint32_t _mipLevels;
		uint32_t _sampleCount;

		uint32_t _width;
		uint32_t _height;
	};
}

#endif