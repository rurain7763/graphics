#include "pch.h"
#include "DXType.h"
#include "DXContext.h"
#include "DXTextures.h"
#include "Graphics/GraphicsFunc.h"
#include "Log/Log.h"

namespace flaw {
	DXTextureCube::DXTextureCube(DXContext& context, const Descriptor& descriptor)
		: _context(context)
	{
		if (!CreateTexture(descriptor)) {
			Log::Error("CreateTexture failed");
			return;
		}

		if (_bindFlags & BindFlag::RenderTarget && !CreateRenderTargetViews()) {
			Log::Error("CreateRenderTargetView failed");
			return;
		}

		if (_bindFlags & BindFlag::DepthStencil && !CreateDepthStencilViews()) {
			Log::Error("CreateDepthStencilView failed");
			return;
		}

		if (_bindFlags & BindFlag::ShaderResource && !CreateShaderResourceView()) {
			Log::Error("CreateShaderResourceView failed");
			return;
		}
	}

	void DXTextureCube::GenerateMips(uint32_t level) {
		if (!(_bindFlags & BindFlag::ShaderResource)) {
			Log::Error("DXTextureCube::GenerateMips: ShaderResource bind flag is required for mip generation");
			return;
		}

		if (level == _mipLevels) {
			return;
		}

		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);
		desc.BindFlags |= D3D11_BIND_RENDER_TARGET; // Ensure render target binding for mip generation
		desc.MipLevels = level;
		desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

		ComPtr<ID3D11Texture2D> newTexture;
		if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, newTexture.GetAddressOf()))) {
			Log::Error("DXTextureCube::GenerateMips: CreateTexture2D failed");
			return;
		}


		for (uint32_t i = 0; i < 6; ++i) {
			_context.DeviceContext()->CopySubresourceRegion(
				newTexture.Get(),
				D3D11CalcSubresource(0, i, level),
				0, 0, 0,
				_texture.Get(),
				D3D11CalcSubresource(0, i, _mipLevels),
				nullptr
			);
		}

		_texture = newTexture;
		_mipLevels = level;

		if (_bindFlags & BindFlag::RenderTarget && !CreateRenderTargetViews()) {
			Log::Error("DXTextureCube::GenerateMips: CreateRenderTargetView failed after mip generation");
			return;
		}

		if (_bindFlags & BindFlag::DepthStencil && !CreateDepthStencilViews()) {
			Log::Error("DXTextureCube::GenerateMips: CreateDepthStencilView failed after mip generation");
			return;
		}

		if (!CreateShaderResourceView()) {
			Log::Error("DXTextureCube::GenerateMips: CreateShaderResourceView failed after mip generation");
			return;
		}

		_context.DeviceContext()->GenerateMips(_srv.Get());
	}

	bool DXTextureCube::CreateTexture(const Descriptor& descriptor) {
		uint32_t faceWidth = descriptor.width;
		uint32_t faceHeight = descriptor.height;

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = faceWidth;
		desc.Height = faceHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 6; // 6 faces for cube map
		desc.Format = ConvertToDXGIFormat(descriptor.format);
		desc.SampleDesc.Count = 1;
		desc.Usage = ConvertD3D11Usage(descriptor.usage);
		desc.BindFlags = ConvertD3D11Bind(descriptor.bindFlags);
		desc.CPUAccessFlags = ConvertD3D11Access(descriptor.access);
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		if (descriptor.data) {
			uint32_t sizePerPixel = GetSizePerPixel(descriptor.format);
			uint32_t rowPitch = faceWidth * sizePerPixel;
			uint32_t fullRowPitch = descriptor.width * sizePerPixel;

			D3D11_SUBRESOURCE_DATA initData[6];
			std::vector<uint8_t> dataBuffer[6];

			struct FacePos { int32_t x, y; };
			FacePos faceCoords[6];

			if (descriptor.layout == Layout::Horizontal) {
				// +---- +---- +---- +---- +---- +---- +
				//| +X || -X || +Y || -Y || +Z || -Z |
				//+---- +---- +---- +---- +---- +---- +
				faceWidth = descriptor.width / 6;

				faceCoords[0] = { 0, 0 }; // +X
				faceCoords[1] = { 1, 0 }; // -X
				faceCoords[2] = { 2, 0 }; // +Y
				faceCoords[3] = { 3, 0 }; // -Y
				faceCoords[4] = { 4, 0 }; // +Z
				faceCoords[5] = { 5, 0 };  // -Z
			}
			else if (descriptor.layout == Layout::HorizontalCross) {
				//		+---- +
				//		| +Y |
				//+----++----++----++---- +
				//| -X || +Z || +X || -Z |
				//+----++----++----++---- +
				//		| -Y |
				//		+---- +
				faceWidth = descriptor.width / 4;
				faceHeight = descriptor.height / 3;

				faceCoords[0] = { 2, 1 }; // +X
				faceCoords[1] = { 0, 1 }; // -X
				faceCoords[2] = { 1, 0 }; // +Y
				faceCoords[3] = { 1, 2 }; // -Y
				faceCoords[4] = { 1, 1 }; // +Z
				faceCoords[5] = { 3, 1 }; // -Z
			}

			for (uint32_t i = 0; i < 6; ++i) {
				const FacePos& faceCoord = faceCoords[i];

				std::vector<uint8_t>& data = dataBuffer[i];
				data.resize(faceWidth * faceHeight * sizePerPixel);

				const uint8_t* src = (const uint8_t*)descriptor.data +
					(faceCoord.y * faceHeight * fullRowPitch) +
					(faceCoord.x * faceWidth * sizePerPixel);

				for (uint32_t y = 0; y < faceHeight; ++y) {
					memcpy(data.data() + y * rowPitch, src, rowPitch);
					src += fullRowPitch;
				}

				initData[i].pSysMem = data.data();
				initData[i].SysMemPitch = rowPitch;
				initData[i].SysMemSlicePitch = 0;
			}

			if (FAILED(_context.Device()->CreateTexture2D(&desc, initData, _texture.GetAddressOf()))) {
				Log::Error("DXTextureCube::CreateTexture: CreateTexture2D failed");
				return false;
			}
		}
		else {
			if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf()))) {
				Log::Error("DXTextureCube::CreateTexture: CreateTexture2D failed");
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

	bool DXTextureCube::CreateRenderTargetViews() {
		_rtvs.clear();

		_rtvs.resize(_mipLevels);
		for (int32_t i = 0; i < _mipLevels; i++) {
			ComPtr<ID3D11RenderTargetView>& rtv = _rtvs[i];

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = ConvertToDXGIFormat(_format);
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = i;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.ArraySize = 6;

			if (FAILED(_context.Device()->CreateRenderTargetView(_texture.Get(), &rtvDesc, rtv.GetAddressOf()))) {
				return false;
			}
		}

		return true;
	}

	bool DXTextureCube::CreateDepthStencilViews() {
		_dsvs.clear();

		_dsvs.resize(_mipLevels);
		for (int32_t i = 0; i < _mipLevels; i++) {
			ComPtr<ID3D11DepthStencilView>& dsv = _dsvs[i];

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = ConvertToDXGIFormat(_format);
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.MipSlice = i;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.ArraySize = 6;

			if (FAILED(_context.Device()->CreateDepthStencilView(_texture.Get(), &dsvDesc, dsv.GetAddressOf()))) {
				return false;
			}
		}

		return true;
	}

	bool DXTextureCube::CreateShaderResourceView() {
		_srv.Reset();

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = ConvertToDXGIFormat(_format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = _mipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;

		if (FAILED(_context.Device()->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf()))) {
			return false;
		}

		return true;
	}
}