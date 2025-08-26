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

		SetRenderPassLayout(context.GetMainRenderPassLayout());
		EnableDepthTest(true);
		SetDepthTest(CompareOp::Less, true);
		EnableStencilTest(false);
		EnableBlendMode(0, true);
		SetBlendMode(0, BlendMode::Default);
		SetAlphaToCoverage(false);
	}

	void DXGraphicsPipeline::SetVertexInputLayouts(const std::vector<Ref<VertexInputLayout>>& vertexInputLayouts) {
		_vertexInputLayouts.clear();
		for (uint32_t i = 0; i < vertexInputLayouts.size(); i++) {
			auto dxVertexInputLayout = std::static_pointer_cast<DXVertexInputLayout>(vertexInputLayouts[i]);
			FASSERT(dxVertexInputLayout, "Invalid vertex input layout");
			
			_vertexInputLayouts.push_back(dxVertexInputLayout);
		}

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

	void DXGraphicsPipeline::EnableDepthTest(bool enable) {
		if (_depthStencilDesc.DepthEnable == enable) {
			return; // No change needed
		}
		
		_depthStencilState = nullptr;

		_depthStencilDesc.DepthEnable = enable;
	}

	void DXGraphicsPipeline::SetDepthTest(CompareOp depthCompareOp, bool depthWrite) {
		D3D11_COMPARISON_FUNC depthFunc = ConvertToDXComparisonFunc(depthCompareOp);
		D3D11_DEPTH_WRITE_MASK writeMask = ConvertToDXDepthWriteMask(depthWrite);
		
		if (depthFunc == _depthStencilDesc.DepthFunc && writeMask == _depthStencilDesc.DepthWriteMask) {
			return;
		}

		_depthStencilState = nullptr;

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

	void DXGraphicsPipeline::EnableStencilTest(bool enable) {
		if (_depthStencilDesc.StencilEnable == enable) {
			return; // No change needed
		}

		_depthStencilState = nullptr;

		_depthStencilDesc.StencilEnable = enable;
	}

	void DXGraphicsPipeline::SetStencilTest(const StencilOperation& frontFace, const StencilOperation& backFace) {
		if (!_depthStencilDesc.StencilEnable) {
			LOG_ERROR("Stencil test is not enabled");
			return;
		}

		if (frontFace.writeMask != backFace.writeMask) {
			LOG_ERROR("DX11 does not support different masks for front and back faces");
			return;
		}

		if (frontFace.compareMask != backFace.compareMask) {
			LOG_ERROR("DX11 does not support different compare masks for front and back faces");
			return;
		}

		if (frontFace.reference != backFace.reference) {
			LOG_ERROR("DX11 does not support different reference values for front and back faces");
			return;
		}

		_depthStencilState = nullptr;

		_depthStencilDesc.FrontFace.StencilFailOp = ConvertToDXStencilOp(frontFace.failOp);
		_depthStencilDesc.FrontFace.StencilDepthFailOp = ConvertToDXStencilOp(frontFace.depthFailOp);
		_depthStencilDesc.FrontFace.StencilPassOp = ConvertToDXStencilOp(frontFace.passOp);
		_depthStencilDesc.FrontFace.StencilFunc = ConvertToDXComparisonFunc(frontFace.compareOp);

		_depthStencilDesc.BackFace.StencilFailOp = ConvertToDXStencilOp(backFace.failOp);
		_depthStencilDesc.BackFace.StencilDepthFailOp = ConvertToDXStencilOp(backFace.depthFailOp);
		_depthStencilDesc.BackFace.StencilPassOp = ConvertToDXStencilOp(backFace.passOp);
		_depthStencilDesc.BackFace.StencilFunc = ConvertToDXComparisonFunc(backFace.compareOp);

		_depthStencilDesc.StencilReadMask = frontFace.compareMask;
		_depthStencilDesc.StencilWriteMask = frontFace.writeMask;

		_stencilRef = frontFace.reference;
	}

	void DXGraphicsPipeline::SetShaderResourcesLayouts(const std::vector<Ref<ShaderResourcesLayout>>& shaderResourceLayouts) {
		_shaderResourcesLayouts.clear();
		for (const auto& layout : shaderResourceLayouts) {
			auto dxLayout = std::static_pointer_cast<DXShaderResourcesLayout>(layout);
			FASSERT(dxLayout, "Invalid shader resource layout");

			_shaderResourcesLayouts.push_back(dxLayout);
		}
	}

	void DXGraphicsPipeline::SetShader(const Ref<GraphicsShader>& shader) {
		if (_shader == shader) {
			return;
		}

		_shader = std::static_pointer_cast<DXGraphicsShader>(shader);
		FASSERT(_shader, "Invalid graphics shader");

		_dxInputLayout = nullptr;
	}

	void DXGraphicsPipeline::SetRenderPassLayout(const Ref<RenderPassLayout>& renderPassLayout) {
		if (_renderPassLayout == renderPassLayout) {
			return;
		}

		_renderPassLayout = std::static_pointer_cast<DXRenderPassLayout>(renderPassLayout);
		FASSERT(_renderPassLayout, "Invalid render pass layout");

		_blendModes.resize(_renderPassLayout->GetColorAttachmentCount());
		for (uint32_t i = 0; i < _blendModes.size(); ++i) {
			_blendModes[i] = BlendMode::Default;
		}
		_alphaToCoverage = false;

		_blendState = nullptr;
	}

	void DXGraphicsPipeline::EnableBlendMode(uint32_t attachmentIndex, bool enable) {
		if (attachmentIndex >= _blendModes.size()) {
			LOG_ERROR("Attachment index out of bounds for blend mode setting");
			return;
		}

		auto& mode = _blendModes[attachmentIndex];

		if (mode.has_value() == enable) {
			return; // No change needed
		}

		if (enable) {
			mode = BlendMode::Default;
		}
		else {
			mode.reset();
		}

		_blendState = nullptr;
	}

	void DXGraphicsPipeline::SetBlendMode(uint32_t attachmentIndex, BlendMode blendMode) {
		if (attachmentIndex >= _blendModes.size()) {
			LOG_ERROR("Attachment index out of bounds for blend mode setting");
			return;
		}

		auto& mode = _blendModes[attachmentIndex];

		if (!mode.has_value()) {
			LOG_ERROR("Blend mode is not enabled for attachment index %d", attachmentIndex);
			return;
		}

		if (mode == blendMode) {
			return;
		}

		mode = blendMode;

		_blendState = nullptr;
	}

	void DXGraphicsPipeline::SetAlphaToCoverage(bool enable) {
		if (_alphaToCoverage == enable) {
			return;
		}

		_alphaToCoverage = enable;

		_blendState = nullptr;
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

		if (_vertexInputLayouts.empty()) {
			return nullptr;
		}

		if (!_shader) {
			return nullptr;
		}

		ComPtr<ID3DBlob> vsBlob = _shader->GetDXVertexShaderBlob();
		if (!vsBlob) {
			LOG_ERROR("Vertex shader blob is null");
			return nullptr;
		}

		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements;
		for (const auto& layout : _vertexInputLayouts) {
			const auto& elements = layout->GetInputElements();
			inputElements.insert(inputElements.end(), elements.begin(), elements.end());
		}

		if (FAILED(_context.Device()->CreateInputLayout(inputElements.data(), inputElements.size(), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), _dxInputLayout.GetAddressOf()))) {
			LOG_ERROR("CreateInputLayout failed");
			return nullptr;
		}

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

	ComPtr<ID3D11BlendState> DXGraphicsPipeline::GetDXBlendState() {
		if (_blendState) {
			return _blendState;
		}

		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.IndependentBlendEnable = TRUE;
		blendDesc.AlphaToCoverageEnable = _alphaToCoverage;

		for (int32_t i = 0; i < _blendModes.size(); ++i) {
			const auto& blendMode = _blendModes[i];
			auto& renderTargetDesc = blendDesc.RenderTarget[i];

			if (!blendMode.has_value()) {
				renderTargetDesc.BlendEnable = FALSE;
				renderTargetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				continue;
			}

			renderTargetDesc.BlendEnable = TRUE;
			ConvertDXBlend(blendMode.value(), renderTargetDesc.SrcBlend, renderTargetDesc.DestBlend);
			renderTargetDesc.BlendOp = D3D11_BLEND_OP_ADD;
			renderTargetDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
			renderTargetDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
			renderTargetDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			renderTargetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		if (FAILED(_context.Device()->CreateBlendState(&blendDesc, &_blendState))) {
			LOG_ERROR("Failed to create blend state.");
		}

		return _blendState;
	}
}

#endif