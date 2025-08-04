#include "pch.h"
#include "DXGraphicsPipeline.h"
#include "DXType.h"
#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXGraphicsPipeline::DXGraphicsPipeline(DXContext& context)
		: _context(context)
		, _depthTest(DepthTest::Less)
		, _depthWrite(true)

	{
		_rasterizerState = CreateRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_BACK);

		_depthStencilState = CreateDepthStencilState(true, D3D11_COMPARISON_LESS, D3D11_DEPTH_WRITE_MASK_ALL);
		if (!_depthStencilState) {
			Log::Error("CreateDepthStencilState failed");
			return;
		}
	}

	void DXGraphicsPipeline::SetDepthTest(DepthTest depthTest, bool depthWrite) {
		if (depthTest == _depthTest && depthWrite == _depthWrite) {
			return;
		}

		_depthTest = depthTest;
		_depthWrite = depthWrite;

		D3D11_COMPARISON_FUNC depthFunc = ConvertD3D11DepthTest(depthTest);
		D3D11_DEPTH_WRITE_MASK writeMask = depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;

		_depthStencilState = CreateDepthStencilState(depthTest != DepthTest::Disabled, depthFunc, writeMask);
	}

	void DXGraphicsPipeline::SetCullMode(CullMode cullMode) {
		if (cullMode == _cullMode) {
			return;
		}

		_cullMode = cullMode;

		_rasterizerState = CreateRasterizerState(GetFillMode(_fillMode), GetCullMode(_cullMode));
	}

	void DXGraphicsPipeline::SetFillMode(FillMode fillMode) {
		if (fillMode == _fillMode) {
			return;
		}

		_fillMode = fillMode;

		_rasterizerState = CreateRasterizerState(GetFillMode(_fillMode), GetCullMode(_cullMode));
	}

	void DXGraphicsPipeline::Bind() {
		_shader->Bind();
		_context.DeviceContext()->RSSetState(_rasterizerState.Get());
		_context.DeviceContext()->OMSetDepthStencilState(_depthStencilState.Get(), 0);
	}

	D3D11_FILL_MODE DXGraphicsPipeline::GetFillMode(FillMode fillMode) {
		switch (fillMode)
		{
		case FillMode::Solid:
			return D3D11_FILL_SOLID;
		case FillMode::Wireframe:
			return D3D11_FILL_WIREFRAME;
		}

		return D3D11_FILL_SOLID;
	}

	D3D11_CULL_MODE DXGraphicsPipeline::GetCullMode(CullMode cullMode) {
		switch (cullMode)
		{
		case CullMode::None:
			return D3D11_CULL_NONE;
		case CullMode::Front:
			return D3D11_CULL_FRONT;
		case CullMode::Back:
			return D3D11_CULL_BACK;
		}

		return D3D11_CULL_BACK;
	}

	ComPtr<ID3D11RasterizerState> DXGraphicsPipeline::CreateRasterizerState(D3D11_FILL_MODE fillMode, D3D11_CULL_MODE cullMode) {
		D3D11_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = fillMode;
		rasterizerDesc.CullMode = cullMode;

		ComPtr<ID3D11RasterizerState> rasterizerState;
		if (FAILED(_context.Device()->CreateRasterizerState(&rasterizerDesc, &rasterizerState))) {
			return nullptr;
		}

		return rasterizerState;
	}

	ComPtr<ID3D11DepthStencilState> DXGraphicsPipeline::CreateDepthStencilState(bool enableDepthTest, D3D11_COMPARISON_FUNC depthTest, D3D11_DEPTH_WRITE_MASK writeMask) {
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = enableDepthTest;
		depthStencilDesc.DepthFunc = depthTest;
		depthStencilDesc.DepthWriteMask = writeMask;
		depthStencilDesc.StencilEnable = FALSE;

		ComPtr<ID3D11DepthStencilState> depthStencilState;
		if (FAILED(_context.Device()->CreateDepthStencilState(&depthStencilDesc, &depthStencilState))) {
			return nullptr;
		}

		return depthStencilState;
	}
}