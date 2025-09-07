#include "pch.h"
#include "DXTextures.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXTextureCube::DXTextureCube(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _format(descriptor.format)
		, _memProperty(descriptor.memProperty)
		, _usages(descriptor.texUsages)
		, _mipLevels(descriptor.mipLevels)
		, _sampleCount(descriptor.sampleCount)
		, _width(descriptor.width)
		, _height(descriptor.height)
	{
		if (!CreateTexture(descriptor.data)) {
			return;
		}

		if (_usages & TextureUsage::ColorAttachment && !CreateRenderTargetViews()) {
			return;
		}

		if (_usages & TextureUsage::DepthStencilAttachment && !CreateDepthStencilViews()) {
			return;
		}

		if (_usages & (TextureUsage::ShaderResource | TextureUsage::InputAttachment)&& !CreateShaderResourceView()) {
			return;
		}

		if (_mipLevels > 1 && _nativeTextureView.srv) {
			_context.DeviceContext()->GenerateMips(_nativeTextureView.srv.Get());
		}
	}

	bool DXTextureCube::CreateTexture(const uint8_t* initRawData) {
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = _width;
		desc.Height = _height;
		desc.MipLevels = _mipLevels;
		desc.ArraySize = 6; // 6 faces for cube map
		desc.Format = IsDepthFormat(_format) ? ConvertDepthFormatToTexFormat(_format) : ConvertToDXFormat(_format);
		desc.SampleDesc.Count = 1;
		desc.Usage = ConvertToDXUsage(_memProperty);
		desc.CPUAccessFlags = ConvertToDXUsage(_memProperty);
		desc.BindFlags = ConvertToDXTexBindFlags(_usages);
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		if (_mipLevels > 1) {
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET; // Required for mip generation
			desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _nativeTexture.texture.GetAddressOf()))) {
			LOG_ERROR("CreateTexture2D failed");
			return false;
		}

		if (initRawData) {
#if false // Reference under line code is for horizontal layout future support
			uint32_t sizePerPixel = GetSizePerPixel(_format);
			uint32_t rowPitch = _width * sizePerPixel;

			struct FacePos { int32_t x, y; };
			FacePos faceCoords[6];
			uint32_t fullRowPitch = 0;

			if (_layout == Layout::Horizontal) {
				// +---- +---- +---- +---- +---- +---- +
				//| +X || -X || +Y || -Y || +Z || -Z |
				//+---- +---- +---- +---- +---- +---- +
				fullRowPitch = rowPitch * 6;

				faceCoords[0] = { 0, 0 }; // +X
				faceCoords[1] = { 1, 0 }; // -X
				faceCoords[2] = { 2, 0 }; // +Y
				faceCoords[3] = { 3, 0 }; // -Y
				faceCoords[4] = { 4, 0 }; // +Z
				faceCoords[5] = { 5, 0 };  // -Z
			}
			else if (_layout == Layout::HorizontalCross) {
				//		+---- +
				//		| +Y |
				//+----++----++----++---- +
				//| -X || +Z || +X || -Z |
				//+----++----++----++---- +
				//		| -Y |
				//		+---- +
				fullRowPitch = rowPitch * 4;

				faceCoords[0] = { 2, 1 }; // +X
				faceCoords[1] = { 0, 1 }; // -X
				faceCoords[2] = { 1, 0 }; // +Y
				faceCoords[3] = { 1, 2 }; // -Y
				faceCoords[4] = { 1, 1 }; // +Z
				faceCoords[5] = { 3, 1 }; // -Z
			}

			for (uint32_t i = 0; i < 6; ++i) {
				const FacePos& faceCoord = faceCoords[i];
				const uint8_t* src = initRawData + (faceCoord.y * _height * fullRowPitch) + (faceCoord.x * rowPitch);
		
				uint32_t subresourceIndex = D3D11CalcSubresource(0, i, _mipLevels);
				_context.DeviceContext()->UpdateSubresource(
					_texture.Get(),
					subresourceIndex,
					nullptr,
					src,
					rowPitch,
					0
				);
			}
#else
			uint32_t sizePerPixel = GetSizePerPixel(_format);
			for (uint32_t i = 0; i < 6; ++i) {
				uint32_t rowPitch = _width * sizePerPixel;

				const uint8_t* src = initRawData + (i * _height * rowPitch);

				uint32_t subresourceIndex = D3D11CalcSubresource(0, i, _mipLevels);
				_context.DeviceContext()->UpdateSubresource(
					_nativeTexture.texture.Get(),
					subresourceIndex,
					nullptr,
					src,
					rowPitch,
					0
				);
			}
#endif
		}

		return true;
	}

	bool DXTextureCube::CreateRenderTargetViews() {
#if false
		_rtvs.resize(_mipLevels);
		for (int32_t i = 0; i < _mipLevels; i++) {
			ComPtr<ID3D11RenderTargetView>& rtv = _rtvs[i];

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = ConvertToDXFormat(_format);
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = i;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.ArraySize = 6;

			if (FAILED(_context.Device()->CreateRenderTargetView(_texture.Get(), &rtvDesc, rtv.GetAddressOf()))) {
				LOG_ERROR("CreateRenderTargetView failed");
				return false;
			}
		}
#else
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = ConvertToDXFormat(_format);
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 6;

		if (FAILED(_context.Device()->CreateRenderTargetView(_nativeTexture.texture.Get(), &rtvDesc, _nativeTextureView.rtv.GetAddressOf()))) {
			LOG_ERROR("CreateRenderTargetView failed");
			return false;
		}
#endif

		return true;
	}

	bool DXTextureCube::CreateDepthStencilViews() {
#if false
		_dsvs.resize(_mipLevels);
		for (int32_t i = 0; i < _mipLevels; i++) {
			ComPtr<ID3D11DepthStencilView>& dsv = _dsvs[i];

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = ConvertToDXFormat(_format);
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.MipSlice = i;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.ArraySize = 6;

			if (FAILED(_context.Device()->CreateDepthStencilView(_texture.Get(), &dsvDesc, dsv.GetAddressOf()))) {
				LOG_ERROR("CreateDepthStencilView failed");
				return false;
			}
		}
#else
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = ConvertToDXFormat(_format);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.ArraySize = 6;

		if (FAILED(_context.Device()->CreateDepthStencilView(_nativeTexture.texture.Get(), &dsvDesc, _nativeTextureView.dsv.GetAddressOf()))) {
			LOG_ERROR("CreateDepthStencilView failed");
			return false;
		}
#endif

		return true;
	}

	bool DXTextureCube::CreateShaderResourceView() {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = IsDepthFormat(_format) ? ConvertDepthFormatToSRVFormat(_format) : ConvertToDXFormat(_format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = _mipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;

		if (FAILED(_context.Device()->CreateShaderResourceView(_nativeTexture.texture.Get(), &srvDesc, _nativeTextureView.srv.GetAddressOf()))) {
			LOG_ERROR("CreateShaderResourceView failed");
			return false;
		}

		return true;
	}
}

#endif