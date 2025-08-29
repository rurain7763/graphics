#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsTextures.h"

namespace flaw {
	class DXContext;

	struct DXNativeTexture : public NativeTexture {
		ComPtr<ID3D11Texture2D> texture;
		ComPtr<ID3D11ShaderResourceView> srv;
		ComPtr<ID3D11UnorderedAccessView> uav;
		ComPtr<ID3D11RenderTargetView> rtv;
		ComPtr<ID3D11DepthStencilView> dsv;
	};

	class DXTexture2D : public Texture2D {
	public:
		DXTexture2D(DXContext& context, const Descriptor& descriptor);
		DXTexture2D(DXContext& context, const ComPtr<ID3D11Texture2D>& texture, const PixelFormat format, uint32_t shaderStages);

		void Fetch(void* outData, const uint32_t size) const override;

		void CopyTo(Ref<Texture2D>& target) const override;
		void CopyToSub(Ref<Texture2D>& target, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) const override;

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _usages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }

		const NativeTexture& GetNativeTexture() const override { return _nativeTexture; }

	private:
		bool CreateTexture(const uint8_t* data);

		bool CreateRenderTargetView();
		bool CreateDepthStencilView();
		bool CreateShaderResourceView();
		bool CreateUnorderedAccessView();

	private:
		DXContext& _context;

		DXNativeTexture _nativeTexture;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _usages;
		uint32_t _mipLevels;
		uint32_t _sampleCount;

		uint32_t _width;
		uint32_t _height;
	};

	class DXTexture2DArray : public Texture2DArray {
	public:
		DXTexture2DArray(DXContext& context, const Descriptor& descriptor);

		void Fetch(void* outData, const uint32_t size) const override;

		void CopyTo(Ref<Texture2DArray>& target) const override;

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _usages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }
		uint32_t GetArraySize() const override { return _arraySize; }

		const NativeTexture& GetNativeTexture() const override { return _nativeTexture; }

	private:
		bool CreateTexture(const uint8_t* data);
		
		bool CreateShaderResourceView();
		bool CreateUnorderedAccessView();
		bool CreateRenderTargetView();
		bool CreateDepthStencilView();

	private:
		DXContext& _context;

		DXNativeTexture _nativeTexture;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _usages;
		uint32_t _mipLevels;
		uint32_t _sampleCount;
		uint32_t _arraySize;

		uint32_t _width;
		uint32_t _height;
	};

	class DXTextureCube : public TextureCube {
	public:
		DXTextureCube(DXContext& context, const Descriptor& descriptor);
		~DXTextureCube() = default;

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _usages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }

		const NativeTexture& GetNativeTexture() const override { return _nativeTexture; }

	private:
		bool CreateTexture(const uint8_t* initRawData);

		bool CreateRenderTargetViews();
		bool CreateDepthStencilViews();
		bool CreateShaderResourceView();

	private:
		DXContext& _context;

		DXNativeTexture _nativeTexture;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _usages;
		uint32_t _mipLevels;
		uint32_t _sampleCount;

		uint32_t _width;
		uint32_t _height;
	};
}

#endif