#include "pch.h"
#include "DXPipelines.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXShaders.h"
#include "DXRenderPass.h"
#include "DXBuffers.h"
#include "Log/Log.h"

namespace flaw {
	DXGraphicsPipeline::DXGraphicsPipeline(DXContext& context)
		: _context(context)
	{
		_depthStencilDesc.StencilEnable = FALSE; // Disable stencil test

		_rasterizerDesc.FrontCounterClockwise = FALSE; // Default is clockwise
		_rasterizerDesc.DepthBias = 0;
		_rasterizerDesc.DepthBiasClamp = 0.0f;
		_rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		_rasterizerDesc.DepthClipEnable = TRUE; // Enable depth clipping
		_rasterizerDesc.ScissorEnable = TRUE; // Enable scissor test
		_rasterizerDesc.MultisampleEnable = FALSE; // Disable multisampling
		_rasterizerDesc.AntialiasedLineEnable = FALSE; // Disable antialiased lines
		_rasterizerDesc.CullMode = D3D11_CULL_BACK; // Default cull mode
		_rasterizerDesc.FillMode = D3D11_FILL_SOLID; // Default fill mode

		_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; // Default primitive topology

		_depthStencilDesc.DepthEnable = TRUE; // Enable depth test by default
		_depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // Default depth function
		_depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // Default depth write mask

		_viewport.TopLeftX = 0.0f;
		_viewport.TopLeftY = 0.0f;
		_viewport.Width = 800.0f; // Default width
		_viewport.Height = 600.0f; // Default height

		_viewport.MinDepth = 0.0f;
		_viewport.MaxDepth = 1.0f; // Default depth range
		_scissorRect.left = 0;
		_scissorRect.top = 0;
		_scissorRect.right = 800; // Default width
		_scissorRect.bottom = 600; // Default height
	}

	void DXGraphicsPipeline::SetVertexInputLayout(const Ref<VertexInputLayout>& vertexInputLayout) {
		if (vertexInputLayout == _vertexInputLayout) {
			return;
		}

		_vertexInputLayout = std::static_pointer_cast<DXVertexInputLayout>(vertexInputLayout);
		FASSERT(_vertexInputLayout, "Invalid vertex input layout");

		_dxInputLayout = nullptr;
	}

	void DXGraphicsPipeline::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
		_primitiveTopology = ConvertToDXPrimitiveTopology(primitiveTopology);
	}

	void DXGraphicsPipeline::SetViewport(float x, float y, float width, float height) {
		_viewport.TopLeftX = x;
		_viewport.TopLeftY = y;
		_viewport.Width = width;
		_viewport.Height = height;
		_viewport.MinDepth = 0.0f;
		_viewport.MaxDepth = 1.0f;
	}

	void DXGraphicsPipeline::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) {
		_scissorRect.left = x;
		_scissorRect.top = y;
		_scissorRect.right = x + width;
		_scissorRect.bottom = y + height;
	}

	void DXGraphicsPipeline::SetDepthTest(DepthTest depthTest, bool depthWrite) {
		D3D11_COMPARISON_FUNC depthFunc = ConvertToDXComparisonFunc(depthTest);
		D3D11_DEPTH_WRITE_MASK writeMask = ConvertToDXDepthWriteMask(depthWrite);
		
		if (depthFunc == _depthStencilDesc.DepthFunc && writeMask == _depthStencilDesc.DepthWriteMask) {
			return;
		}

		_depthStencilState = nullptr;

		_depthStencilDesc.DepthEnable = depthTest != DepthTest::Disabled;
		_depthStencilDesc.DepthFunc = depthFunc;
		_depthStencilDesc.DepthWriteMask = writeMask;
	}

	void DXGraphicsPipeline::SetCullMode(CullMode cullMode) {
		D3D11_CULL_MODE dxCullMode = ConvertToDXCullMode(cullMode);

		if (dxCullMode == _rasterizerDesc.CullMode) {
			return;
		}

		_rasterizerState = nullptr;

		_rasterizerDesc.CullMode = dxCullMode;
	}

	void DXGraphicsPipeline::SetFillMode(FillMode fillMode) {
		D3D11_FILL_MODE dxFillMode = ConverToDXFillMode(fillMode);

		if (dxFillMode == _rasterizerDesc.FillMode) {
			return;
		}

		_rasterizerState = nullptr;

		_rasterizerDesc.FillMode = dxFillMode;
	}

	void DXGraphicsPipeline::AddShaderResourcesLayout(const Ref<ShaderResourcesLayout>& shaderResourceLayout) {
		if (_shaderResourcesLayout) {
			LOG_WARN("DX11 does not support multiple shader resource layouts this job just overrides the previous one");
		}

		_shaderResourcesLayout = std::static_pointer_cast<DXShaderResourcesLayout>(shaderResourceLayout);
		FASSERT(_shaderResourcesLayout, "Invalid shader resource layout");
	}

	void DXGraphicsPipeline::SetShader(const Ref<GraphicsShader>& shader) {
		if (_shader == shader) {
			return;
		}

		_shader = std::static_pointer_cast<DXGraphicsShader>(shader);
		FASSERT(_shader, "Invalid graphics shader");

		_dxInputLayout = nullptr;
	}

	void DXGraphicsPipeline::SetRenderPassLayout(const Ref<GraphicsRenderPassLayout>& renderPassLayout) {
		if (_renderPassLayout == renderPassLayout) {
			return;
		}

		_renderPassLayout = std::static_pointer_cast<DXRenderPassLayout>(renderPassLayout);
		FASSERT(_renderPassLayout, "Invalid render pass layout");
	}

	void DXGraphicsPipeline::SetBehaviorStates(uint32_t flags) {
		_behaviorStates = flags;
	}

	uint32_t DXGraphicsPipeline::GetBehaviorStates() const {
		return _behaviorStates;
	}

	ComPtr<ID3D11InputLayout> DXGraphicsPipeline::GetDXInputLayout() {
		if (_dxInputLayout) {
			return _dxInputLayout;
		}

		if (!_vertexInputLayout) {
			return nullptr;
		}

		if (!_shader) {
			return nullptr;
		}

		_dxInputLayout = _vertexInputLayout->GetDXInputLayout(_shader);

		return _dxInputLayout;
	}

	ComPtr<ID3D11DepthStencilState> DXGraphicsPipeline::GetDXDepthStencilState() {
		if (!_depthStencilState) {
			if (FAILED(_context.Device()->CreateDepthStencilState(&_depthStencilDesc, &_depthStencilState))) {
				LOG_ERROR("Failed to create depth stencil state");
				return nullptr;
			}
		}

		return _depthStencilState;
	}

	ComPtr<ID3D11RasterizerState> DXGraphicsPipeline::GetDXRasterizerState() {
		if (!_rasterizerState) {
			if (FAILED(_context.Device()->CreateRasterizerState(&_rasterizerDesc, &_rasterizerState))) {
				LOG_ERROR("Failed to create rasterizer state");
				return nullptr;
			}
		}

		return _rasterizerState;
	}
}

#endif