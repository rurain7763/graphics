#include "pch.h"
#include "DXType.h"
#include "DXTextures.h"
#include "DXContext.h"
#include "Graphics/GraphicsFunc.h"
#include "Log/Log.h"

namespace flaw {
	DXTexture2DArray::DXTexture2DArray(DXContext& context, const Descriptor& descriptor)
		: _context(context)
	{
		if (!descriptor.fromMemory) {
			if (!SetTexturesUniformAndValidCheck(descriptor.textures)) {
				Log::Error("SetTexturesUniformAndValidCheck failed");
				return;
			}

			if (!CreateTexture(descriptor.textures)) {
				Log::Error("CreateTexture failed");
				return;
			}
		}
		else {
			if (!SetTexturesUniformAndValidCheck(descriptor)) {
				Log::Error("SetTexturesUniformAndValidCheck failed");
				return;
			}

			if (!CreateTexture(descriptor)) {
				Log::Error("CreateTexture failed");
				return;
			}
		}

		if (_bindFlags & BindFlag::RenderTarget) {
			if (!CreateRenderTargetView(_format)) {
				Log::Error("CreateRenderTargetView failed");
				return;
			}
		}

		if (_bindFlags & BindFlag::DepthStencil) {
			if (!CreateDepthStencilView(_format)) {
				Log::Error("CreateDepthStencilView failed");
				return;
			}
		}

		if (_bindFlags & BindFlag::ShaderResource) {
			if (!CreateShaderResourceView(_format)) {
				Log::Error("CreateShaderResourceView failed");
				return;
			}
		}

		if (_bindFlags & BindFlag::UnorderedAccess) {
			if (!CreateUnorderedAccessView(_format)) {
				Log::Error("CreateUnorderedAccessView failed");
				return;
			}
		}
	}

	bool DXTexture2DArray::SetTexturesUniformAndValidCheck(const std::vector<Ref<Texture2D>>& textures) {
		FASSERT(!textures.empty(), "Textures is empty");

		_format = textures[0]->GetPixelFormat();
		_usage = textures[0]->GetUsage();
		_acessFlags = textures[0]->GetAccessFlags();
		_bindFlags = textures[0]->GetBindFlags();
		_width = textures[0]->GetWidth();
		_height = textures[0]->GetHeight();

		for (int32_t i = 1; i < textures.size(); ++i) {
			if (textures[i]->GetPixelFormat() != _format) {
				Log::Error("Textures format is not same");
				return false;
			}
			if (textures[i]->GetUsage() != _usage) {
				Log::Error("Textures usage is not same");
				return false;
			}
			if (textures[i]->GetAccessFlags() != _acessFlags) {
				Log::Error("Textures access flags is not same");
				return false;
			}
			if (textures[i]->GetBindFlags() != _bindFlags) {
				Log::Error("Textures bind flags is not same");
				return false;
			}
			if (textures[i]->GetWidth() != _width) {
				Log::Error("Textures width is not same");
				return false;
			}
			if (textures[i]->GetHeight() != _height) {
				Log::Error("Textures height is not same");
				return false;
			}
		}

		_arraySize = (uint32_t)textures.size();
	}

	bool DXTexture2DArray::SetTexturesUniformAndValidCheck(const Descriptor& descriptor) {
		FASSERT(descriptor.arraySize, "Textures is empty");

		_format = descriptor.format;
		_usage = descriptor.usage;
		_acessFlags = descriptor.access;
		_bindFlags = descriptor.bindFlags;
		_width = descriptor.width;
		_height = descriptor.height;
		_arraySize = descriptor.arraySize;

		return true;
	}

	void DXTexture2DArray::FetchAll(void* outData) const {
		uint8_t* dstPtr = reinterpret_cast<uint8_t*>(outData);

		const uint32_t rowSize = _width * GetSizePerPixel(_format);
		const uint32_t sliceSize = rowSize * _height;

		for (uint32_t slice = 0; slice < _arraySize; ++slice) {
			UINT subresource = D3D11CalcSubresource(0, slice, 1);

			D3D11_MAPPED_SUBRESOURCE mapped = {};
			if (SUCCEEDED(_context.DeviceContext()->Map(_texture.Get(), subresource, D3D11_MAP_READ, 0, &mapped))) {
				uint8_t* dstSliceStart = dstPtr + slice * sliceSize;
				uint8_t* src = reinterpret_cast<uint8_t*>(mapped.pData);

				for (uint32_t y = 0; y < _height; ++y) {
					memcpy(dstSliceStart + y * rowSize, src + y * mapped.RowPitch, rowSize);
				}

				_context.DeviceContext()->Unmap(_texture.Get(), subresource);
			}
			else {
				Log::Error("Failed to map slice %d", slice);
			}
		}
	}

	void DXTexture2DArray::CopyTo(Ref<Texture2DArray>& target) const {
		auto dxTexture = std::static_pointer_cast<DXTexture2DArray>(target);

		for (uint32_t i = 0; i < _arraySize; ++i) {
			UINT offset = D3D11CalcSubresource(0, i, 1);

			_context.DeviceContext()->CopySubresourceRegion(
				dxTexture->GetNativeTexture().Get(),
				offset,
				0, 0, 0,
				_texture.Get(),
				offset,
				nullptr // copy entire slice
			);
		}
	}

	bool DXTexture2DArray::CreateTexture(const std::vector<Ref<Texture2D>>& textures) {
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = _width;
		desc.Height = _height;
		desc.MipLevels = 1;
		desc.ArraySize = _arraySize;
		desc.Format = ConvertToDXGIFormat(_format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = ConvertD3D11Usage(_usage);
		desc.CPUAccessFlags = ConvertD3D11Access(_acessFlags);
		desc.BindFlags = ConvertD3D11Bind(_bindFlags);
		desc.MiscFlags = 0;

		if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf()))) {
			return false;
		}

		for (int32_t i = 0; i < _arraySize; ++i) {
			Ref<DXTexture2D> dxTexture = std::static_pointer_cast<DXTexture2D>(textures[i]);

			_context.DeviceContext()->CopySubresourceRegion(
				_texture.Get(),
				D3D11CalcSubresource(0, i, 1), // dst offset
				0, 0, 0, // dst x,y,z
				dxTexture->GetNativeTexture().Get(),
				0, // subresource index of source
				nullptr // copy entire texture
			);
		}

		return true;
	}

	bool DXTexture2DArray::CreateTexture(const Descriptor& descriptor) {
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = _width;
		desc.Height = _height;
		desc.MipLevels = 1;
		desc.ArraySize = _arraySize;
		desc.Format = ConvertToDXGIFormat(_format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = ConvertD3D11Usage(_usage);
		desc.CPUAccessFlags = ConvertD3D11Access(_acessFlags);
		desc.BindFlags = ConvertD3D11Bind(_bindFlags);
		desc.MiscFlags = 0;

		if (descriptor.data) {
			std::vector<D3D11_SUBRESOURCE_DATA> initDataArray(_arraySize);

			uint32_t slicePitch = _width * _height * GetSizePerPixel(_format);

			for (uint32_t i = 0; i < _arraySize; ++i) {
				initDataArray[i].pSysMem = descriptor.data + i * slicePitch;
				initDataArray[i].SysMemPitch = _width * GetSizePerPixel(_format);
				initDataArray[i].SysMemSlicePitch = slicePitch;
			}

			if (FAILED(_context.Device()->CreateTexture2D(&desc, initDataArray.data(), _texture.GetAddressOf()))) {
				return false;
			}
		}
		else {
			if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf()))) {
				return false;
			}
		}

		return true;
	}

	bool DXTexture2DArray::CreateRenderTargetView(const PixelFormat format) {
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = ConvertToDXGIFormat(format);
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = _arraySize;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(_context.Device()->CreateRenderTargetView(_texture.Get(), &rtvDesc, _rtv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2DArray::CreateDepthStencilView(const PixelFormat format) {
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = ConvertToDXGIFormat(format);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.ArraySize = _arraySize;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(_context.Device()->CreateDepthStencilView(_texture.Get(), &dsvDesc, _dsv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2DArray::CreateShaderResourceView(const PixelFormat format) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = ConvertToDXGIFormat(format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.ArraySize = _arraySize;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;

		if (FAILED(_context.Device()->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2DArray::CreateUnorderedAccessView(const PixelFormat format) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = ConvertToDXGIFormat(format);
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = _arraySize;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(_context.Device()->CreateUnorderedAccessView(_texture.Get(), &uavDesc, _uav.GetAddressOf()))) {
			return false;
		}

		return true;
	}
}

