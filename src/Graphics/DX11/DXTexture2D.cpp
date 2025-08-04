#include "pch.h"
#include "DXType.h"
#include "DXTextures.h"
#include "DXContext.h"
#include "Graphics/GraphicsFunc.h"
#include "Log/Log.h"

namespace flaw {
	DXTexture2D::DXTexture2D(DXContext& context, const Descriptor& descriptor)
		: _context(context)
	{
		if (!CreateTexture(descriptor)) {
			Log::Error("CreateTexture failed");
			return;
		}

		if (_bindFlags & BindFlag::RenderTarget) {
			if (!CreateRenderTargetView()) {
				Log::Error("CreateRenderTargetView failed");
				return;
			}
		}

		if (_bindFlags & BindFlag::DepthStencil) {
			if (!CreateDepthStencilView()) {
				Log::Error("CreateDepthStencilView failed");
				return;
			}
		}

		if (_bindFlags & BindFlag::ShaderResource) {
			if (!CreateShaderResourceView()) {
				Log::Error("CreateShaderResourceView failed");
				return;
			}
		}

		if (_bindFlags & BindFlag::UnorderedAccess) {
			if (!CreateUnorderedAccessView()) {
				Log::Error("CreateUnorderedAccessView failed");
				return;
			}
		}
	}

	DXTexture2D::DXTexture2D(DXContext& context, const ComPtr<ID3D11Texture2D>& texture, const PixelFormat format, const uint32_t bindFlags)
		: _context(context)
	{
		_texture = texture;

		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);

		_format = format;

		if (desc.Usage == D3D11_USAGE_DEFAULT) {
			_usage = UsageFlag::Static;
		}
		else if (desc.Usage == D3D11_USAGE_DYNAMIC) {
			_usage = UsageFlag::Dynamic;
		}
		else if (desc.Usage == D3D11_USAGE_STAGING) {
			_usage = UsageFlag::Staging;
		}
		
		if (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) {
			_acessFlags |= AccessFlag::Read;
		}

		if (desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) {
			_acessFlags |= AccessFlag::Write;
		}

		_bindFlags = bindFlags;
		_mipLevels = desc.MipLevels;

		_width = desc.Width;
		_height = desc.Height;

		if (_bindFlags & BindFlag::RenderTarget) {
			if (!CreateRenderTargetView()) {
				Log::Error("CreateRenderTargetView failed");
				return;
			}
		}
		if (_bindFlags & BindFlag::DepthStencil) {
			if (!CreateDepthStencilView()) {
				Log::Error("CreateDepthStencilView failed");
				return;
			}
		}
		if (_bindFlags & BindFlag::ShaderResource) {
			if (!CreateShaderResourceView()) {
				Log::Error("CreateShaderResourceView failed");
				return;
			}
		}
		if (_bindFlags & BindFlag::UnorderedAccess) {
			if (!CreateUnorderedAccessView()) {
				Log::Error("CreateUnorderedAccessView failed");
				return;
			}
		}
	}

	void DXTexture2D::GenerateMips(uint32_t levels) {
		if (!(_bindFlags & BindFlag::ShaderResource)) {
			Log::Error("GenerateMips called on a texture without ShaderResource bind flag");
			return;
		}

		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);
		desc.BindFlags |= D3D11_BIND_RENDER_TARGET; // Ensure render target binding for mip generation
		desc.MipLevels = levels;
		desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

		ComPtr<ID3D11Texture2D> newTexture;
		if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, newTexture.GetAddressOf()))) {
			Log::Error("CreateTexture2D failed during mip generation");
			return;
		}

		_context.DeviceContext()->CopySubresourceRegion(
			newTexture.Get(), 
			D3D11CalcSubresource(0, 0, levels), 
			0, 0, 0,
			_texture.Get(),
			D3D11CalcSubresource(0, 0, _mipLevels),
			nullptr
		);

		_texture = newTexture;
		_mipLevels = levels;

		if (!CreateShaderResourceView()) {
			Log::Error("CreateShaderResourceView failed after mip generation");
			return;
		}

		_context.DeviceContext()->GenerateMips(_srv.Get());
	}

	void DXTexture2D::Fetch(void* outData, const uint32_t size) const {
		if (_acessFlags & AccessFlag::Read) {
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			if (FAILED(_context.DeviceContext()->Map(_texture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource))) {
				Log::Error("Map failed");
				return;
			}
			memcpy(outData, mappedResource.pData, size);
			_context.DeviceContext()->Unmap(_texture.Get(), 0);
		}
		else {
			Log::Error("Texture is not readable");
		}
	}

	void DXTexture2D::CopyTo(Ref<Texture2D>& target) const {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(target);
		_context.DeviceContext()->CopyResource(dxTexture->GetNativeTexture().Get(), _texture.Get());
	}

	void DXTexture2D::CopyToSub(Ref<Texture2D>& target, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) const {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(target);

		D3D11_BOX box = {};
		box.left = x;
		box.top = y;
		box.right = x + width;
		box.bottom = y + height;
		box.front = 0;
		box.back = 1;

		_context.DeviceContext()->CopySubresourceRegion(
			dxTexture->GetNativeTexture().Get(), 
			0, // dst mip level 
			0, 0, 0, // dst x, y, z
			_texture.Get(), 
			0, // src mip level	
			&box
		);
	}

	bool DXTexture2D::CreateTexture(const Descriptor& descriptor) {
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = descriptor.width;
		desc.Height = descriptor.height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = ConvertToDXGIFormat(descriptor.format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = ConvertD3D11Usage(descriptor.usage);
		desc.CPUAccessFlags = ConvertD3D11Access(descriptor.access);
		desc.BindFlags = ConvertD3D11Bind(descriptor.bindFlags);
		desc.MiscFlags = 0;

		if (descriptor.data) {
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = descriptor.data;
			initData.SysMemPitch = descriptor.width * GetSizePerPixel(descriptor.format);
			initData.SysMemSlicePitch = 0;

			if (FAILED(_context.Device()->CreateTexture2D(&desc, &initData, _texture.GetAddressOf()))) {
				return false;
			}
		}
		else {
			if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf()))) {
				return false;
			}
		}

		_format = descriptor.format;
		_usage = descriptor.usage;
		_acessFlags = descriptor.access;
		_bindFlags = descriptor.bindFlags;
		_mipLevels = desc.MipLevels;

		_width = descriptor.width;
		_height = descriptor.height;

		return true;
	}

	bool DXTexture2D::CreateRenderTargetView() {
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = ConvertToDXGIFormat(_format);
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		if (FAILED(_context.Device()->CreateRenderTargetView(_texture.Get(), &rtvDesc, _rtv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateDepthStencilView() {
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = ConvertToDXGIFormat(_format);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		if (FAILED(_context.Device()->CreateDepthStencilView(_texture.Get(), &dsvDesc, _dsv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateShaderResourceView() {
		_srv.Reset();

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = ConvertToDXGIFormat(_format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = _mipLevels;

		if (FAILED(_context.Device()->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateUnorderedAccessView() {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = ConvertToDXGIFormat(_format);
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

		if (FAILED(_context.Device()->CreateUnorderedAccessView(_texture.Get(), &uavDesc, _uav.GetAddressOf()))) {
			return false;
		}

		return true;
	}
}

