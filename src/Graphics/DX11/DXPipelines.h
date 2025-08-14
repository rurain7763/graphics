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
	class DXRenderPassLayout;
	class DXComputeShader;

	class DXGraphicsPipeline : public GraphicsPipeline {
	public:
		DXGraphicsPipeline(DXContext& context);
		~DXGraphicsPipeline() = default;

		void SetVertexInputLayouts(const std::vector<Ref<VertexInputLayout>>& vertexInputLayouts) override;

		void SetPrimitiveTopology(PrimitiveTopology primitiveTopology) override;
		void SetViewport(float x, float y, float width, float height) override;
		void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) override;

		void SetDepthTest(DepthTest depthTest, bool depthWrite = true) override;
		void SetCullMode(CullMode cullMode) override;
		void SetFillMode(FillMode fillMode) override;

		void SetShaderResourcesLayouts(const std::vector<Ref<ShaderResourcesLayout>>& shaderResourceLayouts) override;
		void SetShader(const Ref<GraphicsShader>& shader) override;

		void SetRenderPassLayout(const Ref<GraphicsRenderPassLayout>& renderPassLayout) override;

		void SetBehaviorStates(uint32_t behaviors) override;
		uint32_t GetBehaviorStates() const override;

		ComPtr<ID3D11InputLayout> GetDXInputLayout();
		inline D3D11_PRIMITIVE_TOPOLOGY GetDXPrimitiveTopology() const { return _primitiveTopology; }
		inline const D3D11_VIEWPORT& GetDXViewport() const { return _viewport; }
		inline const D3D11_RECT& GetDXScissorRect() const { return _scissorRect; }
		ComPtr<ID3D11DepthStencilState> GetDXDepthStencilState();
		ComPtr<ID3D11RasterizerState> GetDXRasterizerState();
		inline Ref<DXShaderResourcesLayout> GetDXShaderResourcesLayout() const { return _shaderResourcesLayout; }
		inline Ref<DXGraphicsShader> GetDXShader() const { return _shader; }
		ComPtr<ID3D11BlendState> GetDXBlendState() const;

	private:
		DXContext& _context;

		std::vector<Ref<DXVertexInputLayout>> _vertexInputLayouts;
		ComPtr<ID3D11InputLayout> _dxInputLayout;
		
		D3D11_PRIMITIVE_TOPOLOGY _primitiveTopology;

		D3D11_VIEWPORT _viewport;
		D3D11_RECT _scissorRect;

		D3D11_DEPTH_STENCIL_DESC _depthStencilDesc;
		ComPtr<ID3D11DepthStencilState> _depthStencilState;

		D3D11_RASTERIZER_DESC _rasterizerDesc;
		ComPtr<ID3D11RasterizerState> _rasterizerState;

		Ref<DXShaderResourcesLayout> _shaderResourcesLayout;

		Ref<DXGraphicsShader> _shader;

		Ref<DXRenderPassLayout> _renderPassLayout;

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