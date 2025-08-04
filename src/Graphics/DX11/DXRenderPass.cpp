#include "pch.h"
#include "DXRenderPass.h"
#include "DXContext.h"
#include "DXTextures.h"
#include "Log/Log.h"
#include "DXType.h"

namespace flaw {
	DXRenderPass::DXRenderPass(DXContext& context, const Descriptor& desc)
		: _context(context) 
		, _dsv(nullptr)
		, _blendState(nullptr)

	{
		if (desc.renderTargets.empty()) {
			Log::Error("DXMultiRenderTarget::DXMultiRenderTarget: No render targets provided.");
			return;
		}

		if (desc.renderTargets.size() > MaxRenderTargets) {
			Log::Error("DXMultiRenderTarget::DXMultiRenderTarget: Too many render targets. Max is %d.", MaxRenderTargets);
			return;
		}

		_renderTargets = desc.renderTargets;
		_depthStencil = desc.depthStencil;

		SetRenderTargetViewsAndViewports();
		SetDepthStencilView();
		CreateBlendState();
	}

	DXRenderPass::~DXRenderPass() {
		Unbind();
	}

	void DXRenderPass::SetRenderTargetViewsAndViewports() {
		_rtvs.resize(_renderTargets.size());
		_viewports.resize(_renderTargets.size());
		for (int32_t i = 0; i < _renderTargets.size(); ++i) {		
			auto& renderTarget = _renderTargets[i];
			auto& rtv = _rtvs[i];
			auto& viewport = _viewports[i];

			rtv = static_cast<ID3D11RenderTargetView*>(renderTarget.texture->GetRenderTargetView(renderTarget.mipLevel));

			viewport.TopLeftX = renderTarget.viewportX;
			viewport.TopLeftY = renderTarget.viewportY;
			viewport.Width = renderTarget.viewportWidth;
			viewport.Height = renderTarget.viewportHeight;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
		}
	}

	void DXRenderPass::SetDepthStencilView() {
		if (!_depthStencil.texture) {
			return;
		}

		_dsv = static_cast<ID3D11DepthStencilView*>(_depthStencil.texture->GetDepthStencilView(_depthStencil.mipLevel));
	}

	void DXRenderPass::Bind(bool clearColor, bool clearDepthStencil) {
		int32_t width, height;
		_context.GetSize(width, height);

		Resize(width, height);

		if (clearColor) {
			ClearAllRenderTargets();
		}

		if (clearDepthStencil) {
			ClearDepthStencil();
		}

		_context.SetRenderPass(this);

		_context.DeviceContext()->OMSetRenderTargets(static_cast<UINT>(_rtvs.size()), _rtvs.data(), _dsv);
		_context.DeviceContext()->RSSetViewports(static_cast<UINT>(_viewports.size()), _viewports.data());
		_context.DeviceContext()->OMSetBlendState(_blendState.Get(), nullptr, 0xffffffff);
	}

	void DXRenderPass::Unbind() {
		if (_context.GetRenderPass() != this) {
			return;
		}

		_context.ResetRenderPass();
	}

	void DXRenderPass::Resize(int32_t width, int32_t height) {
		for (int32_t i = 0; i < _renderTargets.size(); ++i) {
			auto& renderTarget = _renderTargets[i];

			if (renderTarget.texture->GetWidth() == width && renderTarget.texture->GetHeight() == height) {
				continue;
			}

			if (!renderTarget.resizeFunc) {
				continue;
			}

			renderTarget.resizeFunc(renderTarget, width, height);

			auto& rtv = _rtvs[i];
			auto& viewport = _viewports[i];

			rtv = static_cast<ID3D11RenderTargetView*>(renderTarget.texture->GetRenderTargetView(renderTarget.mipLevel));

			viewport.TopLeftX = renderTarget.viewportX;
			viewport.TopLeftY = renderTarget.viewportY;
			viewport.Width = renderTarget.viewportWidth;
			viewport.Height = renderTarget.viewportHeight;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
		}

		if (!_depthStencil.texture) {
			return;
		}

		if (_depthStencil.texture->GetWidth() == width && _depthStencil.texture->GetHeight() == height) {
			return;
		}

		if (!_depthStencil.resizeFunc) {
			return;
		}

		_depthStencil.resizeFunc(_depthStencil, width, height);
		_dsv = static_cast<ID3D11DepthStencilView*>(_depthStencil.texture->GetDepthStencilView(_depthStencil.mipLevel));
	}

	void DXRenderPass::PushRenderTarget(const GraphicsRenderTarget& renderTarget) {
		if (_renderTargets.size() >= MaxRenderTargets) {
			Log::Error("DXMultiRenderTarget::PushRenderTarget: Max render targets reached");
			return;
		}
		_renderTargets.push_back(renderTarget);
		
		SetRenderTargetViewsAndViewports();
		SetDepthStencilView();
		CreateBlendState();
	}

	void DXRenderPass::PopRenderTarget() {
		if (_renderTargets.size() <= 1) {
			Log::Error("DXMultiRenderTarget::PopRenderTarget: No render targets to pop");
			return;
		}
		_renderTargets.pop_back();

		SetRenderTargetViewsAndViewports();
		SetDepthStencilView();
		CreateBlendState();
	}

	void DXRenderPass::SetBlendMode(int32_t slot, BlendMode blendMode, bool alphaToCoverage) {
		FASSERT(slot >= 0 && slot < _renderTargets.size(), "Invalid render target slot");

		auto& renderTarget = _renderTargets[slot];

		if (renderTarget.blendMode == blendMode && renderTarget.alphaToCoverage == alphaToCoverage) {
			return;
		}

		renderTarget.blendMode = blendMode;
		renderTarget.alphaToCoverage = alphaToCoverage;

		CreateBlendState();
	}

	void DXRenderPass::SetViewport(int32_t slot, float x, float y, float width, float height) {
		FASSERT(slot >= 0 && slot < _renderTargets.size(), "Invalid render target slot");

		auto& renderTarget = _renderTargets[slot];
		renderTarget.viewportX = x;
		renderTarget.viewportY = y;
		renderTarget.viewportWidth = width;
		renderTarget.viewportHeight = height;

		auto& viewport = _viewports[slot];
		viewport.TopLeftX = renderTarget.viewportX;
		viewport.TopLeftY = renderTarget.viewportY;
		viewport.Width = renderTarget.viewportWidth;
		viewport.Height = renderTarget.viewportHeight;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
	}

	void DXRenderPass::SetRenderTargetMipLevel(int32_t slot, uint32_t mipLevel) {
		FASSERT(slot >= 0 && slot < _renderTargets.size(), "Invalid render target slot");

		auto& renderTarget = _renderTargets[slot];
		if (renderTarget.mipLevel == mipLevel) {
			return;
		}

		renderTarget.mipLevel = mipLevel;

		auto& rtv = _rtvs[slot];
		rtv = static_cast<ID3D11RenderTargetView*>(renderTarget.texture->GetRenderTargetView(mipLevel));
	}

	void DXRenderPass::SetDepthStencilMipLevel(uint32_t mipLevel) {
		if (!_depthStencil.texture) {
			return;
		}

		if (_depthStencil.mipLevel == mipLevel) {
			return;
		}

		_depthStencil.mipLevel = mipLevel;
		_dsv = static_cast<ID3D11DepthStencilView*>(_depthStencil.texture->GetDepthStencilView(mipLevel));
	}

	Ref<Texture> DXRenderPass::GetRenderTargetTex(int32_t slot) {
		FASSERT(slot >= 0 && slot < _renderTargets.size(), "Invalid render target slot");
		return _renderTargets[slot].texture;
	}

	Ref<Texture> DXRenderPass::GetDepthStencilTex() {
		return _depthStencil.texture;
	}

	void DXRenderPass::ClearAllRenderTargets() {
		for (int32_t i = 0; i < _rtvs.size(); ++i) {
			_context.DeviceContext()->ClearRenderTargetView(_rtvs[i], _renderTargets[i].clearValue.data());
		}
	}

	void DXRenderPass::ClearDepthStencil() {
		if (_dsv) {
			_context.DeviceContext()->ClearDepthStencilView(_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}
	}

	void DXRenderPass::CreateBlendState() {
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.IndependentBlendEnable = TRUE;
		blendDesc.AlphaToCoverageEnable = FALSE;

		for (int32_t i = 0; i < _renderTargets.size(); ++i) {
			auto& renderTarget = _renderTargets[i];
			auto& renderTargetDesc = blendDesc.RenderTarget[i];

			if (renderTarget.blendMode == BlendMode::Disabled) {
				renderTargetDesc.BlendEnable = FALSE;
				renderTargetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				continue;
			}

			renderTargetDesc.BlendEnable = TRUE;
			ConvertD3D11Blend(renderTarget.blendMode, renderTargetDesc.SrcBlend, renderTargetDesc.DestBlend);
			renderTargetDesc.BlendOp = D3D11_BLEND_OP_ADD;
			renderTargetDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
			renderTargetDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
			renderTargetDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			renderTargetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			blendDesc.AlphaToCoverageEnable |= renderTarget.alphaToCoverage;
		}

		if (FAILED(_context.Device()->CreateBlendState(&blendDesc, &_blendState))) {
			Log::Error("DXMultiRenderTarget::CreateBlendState: Failed to create blend state");
			return;
		}
	}

	uint32_t DXRenderPass::GetRenderTargetCount() const {
		return static_cast<uint32_t>(_renderTargets.size());
	}
}