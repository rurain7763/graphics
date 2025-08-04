#pragma once

#include "Graphics/Texture.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <array>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	class DXContext;

	class DXTexture2D : public Texture2D {
	public:
		DXTexture2D(DXContext& context, const Descriptor& descriptor);
		DXTexture2D(DXContext& context, const ComPtr<ID3D11Texture2D>& texture, const PixelFormat format, const uint32_t bindFlags);

		void GenerateMips(uint32_t level) override;

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
		UsageFlag GetUsage() const override { return _usage; }
		uint32_t GetBindFlags() const override { return _bindFlags; }
		uint32_t GetAccessFlags() const override { return _acessFlags; }

		ComPtr<ID3D11Texture2D> GetNativeTexture() const { return _texture; }

	private:
		bool CreateTexture(const Descriptor& descriptor);

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
		UsageFlag _usage;
		uint32_t _acessFlags;
		uint32_t _bindFlags;
		uint32_t _mipLevels;

		uint32_t _width;
		uint32_t _height;
	};

	class DXTexture2DArray : public Texture2DArray {
	public:
		DXTexture2DArray(DXContext& context, const Descriptor& descriptor);

		void FetchAll(void* outData) const override;

		void CopyTo(Ref<Texture2DArray>& target) const override;

		ShaderResourceView GetShaderResourceView() const override { return _srv.Get(); }
		UnorderedAccessView GetUnorderedAccessView() const override { return _uav.Get(); }
		RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const override { return _rtv.Get(); } // TODO: support multiple mip levels
		DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const override { return _dsv.Get(); } // TODO: support multiple mip levels

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		UsageFlag GetUsage() const override { return _usage; }
		uint32_t GetBindFlags() const override { return _bindFlags; }
		uint32_t GetAccessFlags() const override { return _acessFlags; }
		uint32_t GetArraySize() const override { return _arraySize; }

		ComPtr<ID3D11Texture2D> GetNativeTexture() const { return _texture; }

	private:
		bool SetTexturesUniformAndValidCheck(const std::vector<Ref<Texture2D>>& textures);
		bool CreateTexture(const std::vector<Ref<Texture2D>>& textures);
		
		bool SetTexturesUniformAndValidCheck(const Descriptor& descriptor);
		bool CreateTexture(const Descriptor& descriptor);
		
		bool CreateShaderResourceView(const PixelFormat format);
		bool CreateUnorderedAccessView(const PixelFormat format);
		bool CreateRenderTargetView(const PixelFormat format);
		bool CreateDepthStencilView(const PixelFormat format);

	private:
		DXContext& _context;

		ComPtr<ID3D11Texture2D> _texture;

		ComPtr<ID3D11ShaderResourceView> _srv;
		ComPtr<ID3D11UnorderedAccessView> _uav;
		ComPtr<ID3D11RenderTargetView> _rtv;
		ComPtr<ID3D11DepthStencilView> _dsv;

		PixelFormat _format;
		UsageFlag _usage;
		uint32_t _acessFlags;
		uint32_t _bindFlags;

		uint32_t _width;
		uint32_t _height;

		uint32_t _arraySize;
	};

	class DXTextureCube : public TextureCube {
	public:
		DXTextureCube(DXContext& context, const Descriptor& descriptor);
		~DXTextureCube() = default;

		void GenerateMips(uint32_t level) override;

		ShaderResourceView GetShaderResourceView() const override { return _srv.Get(); }
		UnorderedAccessView GetUnorderedAccessView() const override { return nullptr; } // UnorderedAccessView is not supported for cube textures now
		RenderTargetView GetRenderTargetView(uint32_t mipLevel = 0) const override { return mipLevel < _rtvs.size() ? _rtvs[mipLevel].Get() : nullptr; } // TODO: support multiple mip levels
		DepthStencilView GetDepthStencilView(uint32_t mipLevel = 0) const override { return mipLevel < _dsvs.size() ? _dsvs[mipLevel].Get() : nullptr; } // TODO: support multiple mip levels

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }
		PixelFormat GetPixelFormat() const override { return _format; }
		UsageFlag GetUsage() const override { return _usage; }
		uint32_t GetBindFlags() const override { return _bindFlags; }
		uint32_t GetAccessFlags() const override { return _acessFlags; }

		ComPtr<ID3D11Texture2D> GetNativeTexture() const { return _texture; }

	private:
		bool CreateTexture(const Descriptor& descriptor);

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
		UsageFlag _usage;
		uint32_t _acessFlags;
		uint32_t _bindFlags;
		uint32_t _mipLevels;

		uint32_t _width;
		uint32_t _height;
	};
}