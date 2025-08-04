#pragma once

#include "Graphics/GraphicsPipeline.h"

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

	class DXGraphicsPipeline : public GraphicsPipeline {
	public:
		DXGraphicsPipeline(DXContext& context);

		void SetDepthTest(DepthTest depthTest, bool depthWrite = true) override;
		void SetCullMode(CullMode cullMode) override;
		void SetFillMode(FillMode fillMode) override;

		void Bind() override;

	private:
		ComPtr<ID3D11RasterizerState> CreateRasterizerState(D3D11_FILL_MODE fillMode, D3D11_CULL_MODE cullMode);
		ComPtr<ID3D11DepthStencilState> CreateDepthStencilState(bool enableDepthTest, D3D11_COMPARISON_FUNC depthTest, D3D11_DEPTH_WRITE_MASK writeMask);

		static D3D11_FILL_MODE GetFillMode(FillMode fillMode);
		static D3D11_CULL_MODE GetCullMode(CullMode cullMode);

	private:
		DXContext& _context;

		DepthTest _depthTest;
		bool _depthWrite;

		ComPtr<ID3D11RasterizerState> _rasterizerState;
		ComPtr<ID3D11DepthStencilState> _depthStencilState;
	};
}