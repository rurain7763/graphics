#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsPipelines.h"

namespace flaw {
	class DXContext;
	class DXVertexInputLayout;
	class DXShaderResourcesLayout;
	class DXGraphicsShader;
	class DXRenderPass;
	class DXComputeShader;

	class DXGraphicsPipeline : public GraphicsPipeline {
	public:
		DXGraphicsPipeline(DXContext& context);
		~DXGraphicsPipeline() = default;

		void SetVertexInputLayouts(const std::vector<Ref<VertexInputLayout>>& vertexInputLayouts) override;

		void SetPrimitiveTopology(PrimitiveTopology primitiveTopology) override;
		void SetViewport(float x, float y, float width, float height) override;
		void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) override;

		void EnableDepthTest(bool enable) override;
		void SetDepthTest(CompareOp depthCompareOp, bool depthWrite = true) override;

		void SetCullMode(CullMode cullMode) override;
		void SetFillMode(FillMode fillMode) override;

		void EnableStencilTest(bool enable) override;
		void SetStencilTest(const StencilOperation& frontFace, const StencilOperation& backFace) override;

		void SetShaderResourcesLayouts(const std::vector<Ref<ShaderResourcesLayout>>& shaderResourceLayouts) override;
		void SetShader(const Ref<GraphicsShader>& shader) override;

		void SetRenderPass(const Ref<RenderPass>& renderPass, uint32_t subpass) override;
		void EnableBlendMode(uint32_t attachmentIndex, bool enable) override;
		void SetBlendMode(uint32_t attachmentIndex, BlendMode blendMode) override;
		void SetAlphaToCoverage(bool enable) override;

		void SetBehaviorStates(uint32_t behaviors) override;
		uint32_t GetBehaviorStates() const override;

		ComPtr<ID3D11InputLayout> GetDXInputLayout();
		inline D3D11_PRIMITIVE_TOPOLOGY GetDXPrimitiveTopology() const { return _primitiveTopology; }
		inline const D3D11_VIEWPORT& GetDXViewport() const { return _viewport; }
		inline const D3D11_RECT& GetDXScissorRect() const { return _scissorRect; }
		ComPtr<ID3D11DepthStencilState> GetDXDepthStencilState();
		inline uint32_t GetDXStencilRef() const { return _stencilRef; }
		ComPtr<ID3D11RasterizerState> GetDXRasterizerState();
		inline const std::vector<Ref<DXShaderResourcesLayout>>& GetDXShaderResourcesLayouts() const { return _shaderResourcesLayouts; }
		inline Ref<DXGraphicsShader> GetDXShader() const { return _shader; }
		ComPtr<ID3D11BlendState> GetDXBlendState();

	private:
		DXContext& _context;

		std::vector<Ref<DXVertexInputLayout>> _vertexInputLayouts;
		ComPtr<ID3D11InputLayout> _dxInputLayout;
		
		D3D11_PRIMITIVE_TOPOLOGY _primitiveTopology;

		D3D11_VIEWPORT _viewport;
		D3D11_RECT _scissorRect;

		D3D11_DEPTH_STENCIL_DESC _depthStencilDesc;
		uint32_t _stencilRef = 0;
		ComPtr<ID3D11DepthStencilState> _depthStencilState;

		D3D11_RASTERIZER_DESC _rasterizerDesc;
		ComPtr<ID3D11RasterizerState> _rasterizerState;

		std::vector<Ref<DXShaderResourcesLayout>> _shaderResourcesLayouts;

		Ref<DXGraphicsShader> _shader;

		Ref<DXRenderPass> _renderPass;
		uint32_t _subpass;

		std::vector<std::optional<BlendMode>> _blendModes;
		bool _alphaToCoverage = false;
		ComPtr<ID3D11BlendState> _blendState;

		uint32_t _behaviorStates = 0;
	};

	class DXComputePipeline : public ComputePipeline {
	public:
		DXComputePipeline(DXContext& context);
		~DXComputePipeline() = default;

		void SetShader(const Ref<ComputeShader>& shader) override;
		void AddShaderResourcesLayout(const Ref<ShaderResourcesLayout>& shaderResourceLayout) override;

		inline Ref<DXComputeShader> GetDXShader() const { return _shader; }

	private:
		DXContext& _context;

		Ref<DXComputeShader> _shader;
	};
}

#endif