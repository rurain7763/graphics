#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsTextures.h"

#include <array>

namespace flaw {
	class VkContext;

	class VkTexture2D : public Texture2D {
	public:
		VkTexture2D(VkContext& context, const Descriptor& descriptor);
		VkTexture2D(VkContext& context, vk::Image image, PixelFormat format, uint32_t bindFlags);
		~VkTexture2D();

		void GenerateMips(uint32_t level) override {}

		void Fetch(void* outData, const uint32_t size) const override {}

		void CopyTo(Ref<Texture2D>& target) const override {}
		void CopyToSub(Ref<Texture2D>& target, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) const override {}

		ShaderResourceView GetShaderResourceView() const override { return (void*)&_imageView; }
		UnorderedAccessView GetUnorderedAccessView() const override { return (void*)&_imageView; }
		RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const override { return (void*)&_imageView; }
		DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const override { return (void*)&_imageView; }

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		UsageFlag GetUsage() const override { return _usage; }
		uint32_t GetBindFlags() const override { return _bindFlags; }
		uint32_t GetAccessFlags() const override { return _acessFlags; }

		inline vk::Image GetVkImage() const { return _image; }
		inline vk::ImageView GetVkImageView() const { return _imageView; }
		inline const vk::DescriptorImageInfo& GetVkDescriptorImageInfo() const { return _imageInfo; }

	private:
		bool CreateImage(bool hasData);
		bool AllocateMemory();
		bool PullMemory(const uint8_t* data);

		bool CreateImageView();
		bool CreateSampler();

	private:
		VkContext& _context;

		bool _isExternalImage = false;

		vk::Image _image;
		vk::DeviceMemory _imageMemory;
		vk::ImageView _imageView;
		vk::Sampler _sampler;
		vk::DescriptorImageInfo _imageInfo;

		PixelFormat _format;
		UsageFlag _usage;
		uint32_t _acessFlags;
		uint32_t _bindFlags;
		uint32_t _mipLevels;

		uint32_t _width;
		uint32_t _height;
	};

	class VkTextureCube : public TextureCube {
	public:
		VkTextureCube(VkContext& context, const Descriptor& descriptor);
		~VkTextureCube();

		void GenerateMips(uint32_t level) override {} 

		ShaderResourceView GetShaderResourceView() const override { return _imageView; }
		UnorderedAccessView GetUnorderedAccessView() const override { return _imageView; }
		RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const override { return _imageView; }
		DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const override { return _imageView; }

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		UsageFlag GetUsage() const override { return _usage; }
		uint32_t GetBindFlags() const override { return _bindFlags; }
		uint32_t GetAccessFlags() const override { return _acessFlags; }

		inline vk::Image GetVkImage() const { return _image; }
		inline vk::ImageView GetVkImageView() const { return _imageView; }
		inline const vk::DescriptorImageInfo& GetVkDescriptorImageInfo() const { return _imageInfo; }

	private:
		bool CreateImage(bool hasData);
		bool AllocateMemory();
		bool PullMemory(const uint8_t* data);

		bool CreateImageView();
		bool CreateSampler();

	private:
		VkContext& _context;

		vk::Image _image;
		vk::DeviceMemory _imageMemory;
		vk::ImageView _imageView;
		vk::Sampler _sampler;
		vk::DescriptorImageInfo _imageInfo;

		PixelFormat _format;
		UsageFlag _usage;
		uint32_t _acessFlags;
		uint32_t _bindFlags;
		uint32_t _mipLevels;

		uint32_t _width;
		uint32_t _height;
	};
}

#endif