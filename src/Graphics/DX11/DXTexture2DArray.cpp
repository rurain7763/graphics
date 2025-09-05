#include "pch.h"
#include "DXTextures.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXTexture2DArray::DXTexture2DArray(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _format(descriptor.format)
		, _memProperty(descriptor.memProperty)
		, _usages(descriptor.texUsages)
		, _mipLevels(descriptor.mipLevels)
		, _sampleCount(descriptor.sampleCount)
		, _width(descriptor.width)
		, _height(descriptor.height)
		, _arraySize(descriptor.arraySize)
	{
		if (!CreateTexture(descriptor.data)) {
			return;
		}

		if (_usages & TextureUsage::ColorAttachment) {
			if (!CreateRenderTargetView()) {
				return;
			}
		}

		if (_usages & TextureUsage::DepthStencilAttachment) {
			if (!CreateDepthStencilView()) {
				return;
			}
		}

		if (_usages & (TextureUsage::ShaderResource | TextureUsage::InputAttachment)) {
			if (!CreateShaderResourceView()) {
				return;
			}
		}

		if (_usages & TextureUsage::UnorderedAccess) {
			if (!CreateUnorderedAccessView()) {
				return;
			}
		}
	}

	bool DXTexture2DArray::CreateTexture(const uint8_t* data) {
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = _width;
		desc.Height = _height;
		desc.MipLevels = _mipLevels;
		desc.ArraySize = _arraySize;
		desc.Format = ConvertToDXFormat(_format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = ConvertToDXUsage(_memProperty);
		desc.CPUAccessFlags = ConvertToDXCPUAccessFlags(_memProperty);
		desc.BindFlags = ConvertToDXTexBindFlags(_usages);
		desc.MiscFlags = 0;

		if (data) {
			std::vector<D3D11_SUBRESOURCE_DATA> initDataArray(_arraySize);

			uint32_t slicePitch = _width * _height * GetSizePerPixel(_format);

			for (uint32_t i = 0; i < _arraySize; ++i) {
				initDataArray[i].pSysMem = data + i * slicePitch;
				initDataArray[i].SysMemPitch = _width * GetSizePerPixel(_format);
				initDataArray[i].SysMemSlicePitch = slicePitch;
			}

			if (FAILED(_context.Device()->CreateTexture2D(&desc, initDataArray.data(), _nativeTexture.texture.GetAddressOf()))) {
				return false;
			}
		}
		else {
			if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _nativeTexture.texture.GetAddressOf()))) {
				return false;
			}
		}

		return true;
	}

	bool DXTexture2DArray::CreateRenderTargetView() {
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = ConvertToDXFormat(_format);
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = _arraySize;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(_context.Device()->CreateRenderTargetView(_nativeTexture.texture.Get(), &rtvDesc, _nativeTextureView.rtv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2DArray::CreateDepthStencilView() {
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = ConvertToDXFormat(_format);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.ArraySize = _arraySize;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(_context.Device()->CreateDepthStencilView(_nativeTexture.texture.Get(), &dsvDesc, _nativeTextureView.dsv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2DArray::CreateShaderResourceView() {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = ConvertToDXFormat(_format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.ArraySize = _arraySize;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;

		if (FAILED(_context.Device()->CreateShaderResourceView(_nativeTexture.texture.Get(), &srvDesc, _nativeTextureView.srv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2DArray::CreateUnorderedAccessView() {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = ConvertToDXFormat(_format);
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = _arraySize;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = 0;

		if (FAILED(_context.Device()->CreateUnorderedAccessView(_nativeTexture.texture.Get(), &uavDesc, _nativeTextureView.uav.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	void DXTexture2DArray::Fetch(void* outData, const uint32_t size) const {
		if (_memProperty != MemoryProperty::Staging) {
			LOG_ERROR("Cannot fetch data from a non-staging texture");
			return;
		}

		uint8_t* dstPtr = reinterpret_cast<uint8_t*>(outData);

		const uint32_t rowSize = _width * GetSizePerPixel(_format);
		const uint32_t sliceSize = rowSize * _height;

		if (size < _arraySize * sliceSize) {
			LOG_ERROR("Provided size is too small to hold the texture data");
			return;
		}

		for (uint32_t slice = 0; slice < _arraySize; ++slice) {
			UINT subresource = D3D11CalcSubresource(0, slice, _mipLevels);

			D3D11_MAPPED_SUBRESOURCE mapped = {};
			if (SUCCEEDED(_context.DeviceContext()->Map(_nativeTexture.texture.Get(), subresource, D3D11_MAP_READ, 0, &mapped))) {
				uint8_t* dstSliceStart = dstPtr + slice * sliceSize;
				uint8_t* src = reinterpret_cast<uint8_t*>(mapped.pData);

				for (uint32_t y = 0; y < _height; ++y) {
					memcpy(dstSliceStart + y * rowSize, src + y * mapped.RowPitch, rowSize);
				}

				_context.DeviceContext()->Unmap(_nativeTexture.texture.Get(), subresource);
			}
			else {
				LOG_ERROR("Failed to map slice %d", slice);
			}
		}
	}

	void DXTexture2DArray::CopyTo(Ref<Texture2DArray>& target) const {
		auto dxTexture = std::static_pointer_cast<DXTexture2DArray>(target);

		for (uint32_t i = 0; i < _arraySize; ++i) {
			UINT offset = D3D11CalcSubresource(0, i, 1);

			_context.DeviceContext()->CopySubresourceRegion(
				dxTexture->_nativeTexture.texture.Get(),
				offset,
				0, 0, 0,
				_nativeTexture.texture.Get(),
				offset,
				nullptr // copy entire slice
			);
		}
	}
}

#endif
