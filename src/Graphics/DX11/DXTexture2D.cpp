#include "pch.h"
#include "DXTextures.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXTexture2D::DXTexture2D(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _memProperty(descriptor.memProperty)
		, _format(descriptor.format)
		, _usages(descriptor.texUsages)
		, _mipLevels(descriptor.mipLevels)
		, _sampleCount(descriptor.sampleCount)
		, _shaderStages(descriptor.shaderStages)
		, _width(descriptor.width)
		, _height(descriptor.height)
	{
		if (!CreateTexture(descriptor.data)) {
			return;
		}

		if (_usages & TextureUsage::RenderTarget) {
			if (!CreateRenderTargetView()) {
				Log::Error("CreateRenderTargetView failed");
				return;
			}
		}

		if (_usages & TextureUsage::DepthStencil) {
			if (!CreateDepthStencilView()) {
				Log::Error("CreateDepthStencilView failed");
				return;
			}
		}

		if (_usages & TextureUsage::ShaderResource) {
			if (!CreateShaderResourceView()) {
				Log::Error("CreateShaderResourceView failed");
				return;
			}

			if (_mipLevels > 1) {
				_context.DeviceContext()->GenerateMips(_srv.Get());
			}
		}

		if (_usages & TextureUsage::UnorderedAccess) {
			if (!CreateUnorderedAccessView()) {
				Log::Error("CreateUnorderedAccessView failed");
				return;
			}
		}
	}

	DXTexture2D::DXTexture2D(DXContext& context, const ComPtr<ID3D11Texture2D>& texture, const PixelFormat format, uint32_t shaderStages)
		: _context(context)
	{
		_texture = texture;

		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);

		_format = ConvertToPixelFormat(desc.Format);
		_memProperty = ConvertToMemoryProperty(desc.Usage);
		_usages = ConvertToTexUsages(desc.BindFlags);

		_mipLevels = desc.MipLevels;

		_shaderStages = shaderStages;

		_width = desc.Width;
		_height = desc.Height;

		if (_usages & TextureUsage::RenderTarget) {
			if (!CreateRenderTargetView()) {
				return;
			}
		}

		if (_usages & TextureUsage::DepthStencil) {
			if (!CreateDepthStencilView()) {
				return;
			}
		}

		if (_usages & TextureUsage::ShaderResource) {
			if (!CreateShaderResourceView()) {
				return;
			}

			if (_mipLevels > 1) {
				_context.DeviceContext()->GenerateMips(_srv.Get());
			}
		}

		if (_usages & TextureUsage::UnorderedAccess) {
			if (!CreateUnorderedAccessView()) {
				return;
			}
		}
	}

	void DXTexture2D::Fetch(void* outData, const uint32_t size) const {
		if (_memProperty != MemoryProperty::Staging) {
			Log::Error("Fetch called on a non-staging texture");
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (FAILED(_context.DeviceContext()->Map(_texture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource))) {
			Log::Error("Map failed");
			return;
		}

		memcpy(outData, mappedResource.pData, size);

		_context.DeviceContext()->Unmap(_texture.Get(), 0);
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
			0,
			0, 0, 0, // dst x, y, z
			_texture.Get(), 
			0,
			&box
		);
	}

	bool DXTexture2D::CreateTexture(const uint8_t* data) {
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = _width;
		desc.Height = _height;
		desc.MipLevels = _mipLevels;
		desc.ArraySize = 1;
		desc.Format = ConvertToDXFormat(_format);
		desc.SampleDesc.Count = _sampleCount;
		desc.SampleDesc.Quality = 0;
		desc.Usage = ConvertToDXUsage(_memProperty);
		desc.CPUAccessFlags = ConvertToDXCPUAccessFlags(_memProperty);
		desc.BindFlags = ConvertToDXTexBindFlags(_usages);
		desc.MiscFlags = 0;

		if (_mipLevels > 1) {
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf()))) {
			LOG_ERROR("Failed to create texture2D without initial data");
			return false;
		}

		if (data) {
			_context.DeviceContext()->UpdateSubresource(
				_texture.Get(),
				0, // Mip level
				nullptr, // D3D11_BOX
				data,
				_width * GetSizePerPixel(_format), // Row pitch
				0 // Depth pitch
			);
		}

		return true;
	}

	bool DXTexture2D::CreateRenderTargetView() {
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = ConvertToDXFormat(_format);
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		if (FAILED(_context.Device()->CreateRenderTargetView(_texture.Get(), &rtvDesc, _rtv.GetAddressOf()))) {
			LOG_ERROR("CreateRenderTargetView failed");
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateDepthStencilView() {
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = ConvertToDXFormat(_format);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		if (FAILED(_context.Device()->CreateDepthStencilView(_texture.Get(), &dsvDesc, _dsv.GetAddressOf()))) {
			LOG_ERROR("CreateDepthStencilView failed");
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateShaderResourceView() {
		_srv.Reset();

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = ConvertToDXFormat(_format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = _mipLevels;

		if (FAILED(_context.Device()->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf()))) {
			LOG_ERROR("CreateShaderResourceView failed");
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateUnorderedAccessView() {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = ConvertToDXFormat(_format);
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

		if (FAILED(_context.Device()->CreateUnorderedAccessView(_texture.Get(), &uavDesc, _uav.GetAddressOf()))) {
			LOG_ERROR("CreateUnorderedAccessView failed");
			return false;
		}

		return true;
	}
}

#endif

