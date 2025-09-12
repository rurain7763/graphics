#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsCommandQueue.h"

namespace flaw {
	class DXRenderPass;
	class DXFramebuffer;

	class FAPI DXContext : public GraphicsContext {
	public:
		DXContext(PlatformContext& context, int32_t width, int32_t height, bool msaa = false);
		~DXContext();

		bool Prepare() override;

		Ref<VertexInputLayout> CreateVertexInputLayout(const VertexInputLayout::Descriptor& descriptor) override;

		Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) override;
		Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) override;

		Ref<ShaderResourcesLayout> CreateShaderResourcesLayout(const ShaderResourcesLayout::Descriptor& descriptor) override;
		Ref<ShaderResources> CreateShaderResources(const ShaderResources::Descriptor& descriptor) override;
		Ref<GraphicsShader> CreateGraphicsShader(const GraphicsShader::Descriptor& descriptor) override;
		Ref<GraphicsPipeline> CreateGraphicsPipeline() override;

		Ref<ConstantBuffer> CreateConstantBuffer(const ConstantBuffer::Descriptor& desc) override;
		Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) override;

		Ref<Texture2D> CreateTexture2D(const Texture2D::Descriptor& descriptor) override;
		Ref<Texture2DArray> CreateTexture2DArray(const Texture2DArray::Descriptor& descriptor) override;
		Ref<TextureCube> CreateTextureCube(const TextureCube::Descriptor& descriptor) override;
		Ref<TextureCubeArray> CreateTextureCubeArray(const TextureCubeArray::Descriptor& descriptor) override;

		Ref<RenderPass> CreateRenderPass(const RenderPass::Descriptor& desc) override;
		Ref<Framebuffer> CreateFramebuffer(const Framebuffer::Descriptor& desc) override;

		uint32_t GetFrameCount() const override;
		uint32_t GetCurrentFrameIndex() const override;

		Ref<Texture2D> GetFrameColorAttachment(uint32_t frameIndex) const override;

		GraphicsCommandQueue& GetCommandQueue() override;

		void Resize(int32_t width, int32_t height) override;
		void GetSize(int32_t& width, int32_t& height) override;
		uint32_t GetMSAASampleCount() const override;

		PixelFormat GetSurfaceFormat() const override;
		PixelFormat GetDepthStencilFormat() const override;

		Ref<ComputeShader> CreateComputeShader(const ComputeShader::Descriptor& descriptor) override;
		Ref<ComputePipeline> CreateComputePipeline() override;

		void AddOnResizeHandler(uint32_t id, const std::function<void(int32_t, int32_t)>& handler);
		void RemoveOnResizeHandler(uint32_t id);

		inline ComPtr<ID3D11Device> Device() const { return _device; }
		inline ComPtr<ID3D11DeviceContext> DeviceContext() const { return _deviceContext; }
		inline ComPtr<IDXGISwapChain> DXSwapChain() const { return _swapChain; }

	private:
		int32_t CreateSwapChain();

		ID3D11SamplerState* CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v, D3D11_TEXTURE_ADDRESS_MODE w);

	private:
		HWND _hWnd;

		int32_t _renderWidth, _renderHeight;
		bool _enableMSAA;

		uint32_t _msaaSampleCount;

		ComPtr<ID3D11Device> _device;
		ComPtr<ID3D11DeviceContext> _deviceContext;

		ComPtr<IDXGISwapChain> _swapChain;
		Ref<Texture2D> _colorAttachment;

		std::array<ID3D11SamplerState*, 2> _samplerStates;

		Ref<GraphicsCommandQueue> _commandQueue;

		std::unordered_map<uint32_t, std::list<std::function<void(int32_t, int32_t)>>::iterator> _onResizeHandlerIters;
		std::list<std::function<void(int32_t, int32_t)>> _onResizeHandlers;
	};
}

#endif