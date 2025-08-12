#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsTextures.h"

namespace flaw {
	class DXContext;

	class DXTexture2D : public Texture2D {
	public:
		DXTexture2D(DXContext& context, const Descriptor& descriptor);
		DXTexture2D(DXContext& context, const ComPtr<ID3D11Texture2D>& texture, const PixelFormat format, uint32_t shaderStages);

		void Fetch(void* outData, const uint32_t size) const override;

		void CopyTo(Ref<Texture2D>& target) const override;
		void CopyToSub(Ref<Texture2D>& target, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) const override;

		ShaderResourceView GetShaderResourceView() const override { return _srv.Get(); }
		UnorderedAccessView GetUnorderedAccessView() const override { return _uav.Get(); }
		RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const override { return _rtv.Get(); }
		DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const override { return _dsv.Get(); }

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _usages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }
		ShaderStages GetShaderStages() const override { return _shaderStages; }

		inline ComPtr<ID3D11Texture2D> GetNativeTexture() const { return _texture; }
		inline ComPtr<ID3D11RenderTargetView> GetNativeRTV() const { return _rtv; }
		inline ComPtr<ID3D11DepthStencilView> GetNativeDSV() const { return _dsv; }
		inline ComPtr<ID3D11ShaderResourceView> GetNativeSRV() const { return _srv; }
		inline ComPtr<ID3D11UnorderedAccessView> GetNativeUAV() const { return _uav; }

	private:
		bool CreateTexture(const uint8_t* data);

		bool CreateRenderTargetView();
		bool CreateDepthStencilView();
		bool CreateShaderResourceView();
		bool CreateUnorderedAccessView();

	private:
		DXContext& _context;

		ComPtr<ID3D11Texture2D> _texture;

		ComPtr<ID3D11RenderTargetView> _rtv;
		ComPtr<ID3D11DepthStencilView> _dsv;
		ComPtr<ID3D11ShaderResourceView> _srv;
		ComPtr<ID3D11UnorderedAccessView> _uav;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _usages;
		uint32_t _mipLevels;
		uint32_t _sampleCount;
		ShaderStages _shaderStages;

		uint32_t _width;
		uint32_t _height;
	};

	class DXTexture2DArray : public Texture2DArray {
	public:
		DXTexture2DArray(DXContext& context, const Descriptor& descriptor);

		void Fetch(void* outData, const uint32_t size) const override;

		void CopyTo(Ref<Texture2DArray>& target) const override;

		ShaderResourceView GetShaderResourceView() const override { return _srv.Get(); }
		UnorderedAccessView GetUnorderedAccessView() const override { return _uav.Get(); }
		RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const override { return _rtv.Get(); } // TODO: support multiple mip levels
		DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const override { return _dsv.Get(); } // TODO: support multiple mip levels

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _usages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }
		ShaderStages GetShaderStages() const override { return _shaderStages; }
		uint32_t GetArraySize() const override { return _arraySize; }

		ComPtr<ID3D11Texture2D> GetNativeTexture() const { return _texture; }

	private:
		bool CreateTexture(const uint8_t* data);
		
		bool CreateShaderResourceView();
		bool CreateUnorderedAccessView();
		bool CreateRenderTargetView();
		bool CreateDepthStencilView();

	private:
		DXContext& _context;

		ComPtr<ID3D11Texture2D> _texture;

		ComPtr<ID3D11ShaderResourceView> _srv;
		ComPtr<ID3D11UnorderedAccessView> _uav;
		ComPtr<ID3D11RenderTargetView> _rtv;
		ComPtr<ID3D11DepthStencilView> _dsv;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _usages;
		uint32_t _mipLevels;
		uint32_t _sampleCount;
		ShaderStages _shaderStages;
		uint32_t _arraySize;

		uint32_t _width;
		uint32_t _height;
	};

	class DXTextureCube : public TextureCube {
	public:
		DXTextureCube(DXContext& context, const Descriptor& descriptor);
		~DXTextureCube() = default;

		ShaderResourceView GetShaderResourceView() const override { return _srv.Get(); }
		UnorderedAccessView GetUnorderedAccessView() const override { return nullptr; } // UnorderedAccessView is not supported for cube textures now
		RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const override { return mipLevel < _rtvs.size() ? _rtvs[mipLevel].Get() : nullptr; } // TODO: support multiple mip levels
		DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const override { return mipLevel < _dsvs.size() ? _dsvs[mipLevel].Get() : nullptr; } // TODO: support multiple mip levels

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		TextureUsages GetUsages() const override { return _usages; }
		uint32_t GetSampleCount() const override { return _sampleCount; }
		ShaderStages GetShaderStages() const override { return _shaderStages; }

		inline ComPtr<ID3D11Texture2D> GetNativeTexture() const { return _texture; }
		inline ComPtr<ID3D11ShaderResourceView> GetNativeSRV() const { return _srv; }

	private:
		bool CreateTexture(const uint8_t* initRawData);

		bool CreateRenderTargetViews();
		bool CreateDepthStencilViews();
		bool CreateShaderResourceView();

	private:
		DXContext& _context;

		ComPtr<ID3D11Texture2D> _texture;

		std::vector<ComPtr<ID3D11RenderTargetView>> _rtvs; // rtvs per mip level
		std::vector<ComPtr<ID3D11DepthStencilView>> _dsvs; // dsvs per mip level
		ComPtr<ID3D11ShaderResourceView> _srv;

		PixelFormat _format;
		MemoryProperty _memProperty;
		TextureUsages _usages;
		uint32_t _mipLevels;
		uint32_t _sampleCount;
		ShaderStages _shaderStages;

		uint32_t _width;
		uint32_t _height;
	};
}

#endif