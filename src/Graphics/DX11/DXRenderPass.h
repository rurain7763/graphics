#pragma once

#include "Graphics/GraphicsRenderPass.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	class DXContext;
	class DXTexture2D;

	class DXRenderPass : public GraphicsRenderPass {
	public:
		DXRenderPass(DXContext& context, const Descriptor& desc);
		~DXRenderPass();

		void Bind(bool clearColor = true, bool clearDepthStencil = true) override;
		void Unbind() override;

		void Resize(int32_t width, int32_t height) override;

		void PushRenderTarget(const GraphicsRenderTarget& renderTarget) override;
		void PopRenderTarget() override;

		void SetBlendMode(int32_t slot, BlendMode blendMode, bool alphaToCoverage) override;
		void SetViewport(int32_t slot, float x, float y, float width, float height) override;

		void SetRenderTargetMipLevel(int32_t slot, uint32_t mipLevel) override;
		void SetDepthStencilMipLevel(uint32_t mipLevel) override;

		Ref<Texture> GetRenderTargetTex(int32_t slot) override;
		Ref<Texture> GetDepthStencilTex() override;

		void ClearAllRenderTargets() override;
		void ClearDepthStencil() override;

		uint32_t GetRenderTargetCount() const override;

	private:
		void SetRenderTargetViewsAndViewports();
		void SetDepthStencilView();

		void CreateBlendState();

	private:
		static constexpr uint32_t MaxRenderTargets = 8;

		DXContext& _context;

		std::vector<GraphicsRenderTarget> _renderTargets;
		std::vector<ID3D11RenderTargetView*> _rtvs;
		std::vector<D3D11_VIEWPORT> _viewports;

		GraphicsDepthStencil _depthStencil;
		ID3D11DepthStencilView* _dsv;

		ComPtr<ID3D11BlendState> _blendState;
	};
}