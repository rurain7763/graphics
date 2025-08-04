#pragma once

#ifdef _WIN32

#include "Core.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsCommandQueue.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <array>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	class FAPI DXContext : public GraphicsContext {
	public:
		DXContext(PlatformContext& context, int32_t width, int32_t height);
		~DXContext();

		void Prepare() override;
		void Present() override;

		Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) override;
		Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) override;
		Ref<GraphicsShader> CreateGraphicsShader(const char* filePath, const uint32_t compileFlag) override;
		Ref<GraphicsPipeline> CreateGraphicsPipeline() override;

		Ref<ConstantBuffer> CreateConstantBuffer(uint32_t size) override;

		Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) override;

		Ref<Texture2D> CreateTexture2D(const Texture2D::Descriptor& descriptor) override;
		Ref<Texture2DArray> CreateTexture2DArray(const Texture2DArray::Descriptor& descriptor) override;
		Ref<TextureCube> CreateTextureCube(const TextureCube::Descriptor& descriptor) override;

		Ref<GraphicsRenderPass> GetMainRenderPass() override;

		GraphicsCommandQueue& GetCommandQueue() override;

		Ref<GraphicsRenderPass> CreateRenderPass(const GraphicsRenderPass::Descriptor& desc) override;

		void Resize(int32_t width, int32_t height) override;
		void GetSize(int32_t& width, int32_t& height) override;

		Ref<ComputeShader> CreateComputeShader(const char* filename) override;
		Ref<ComputePipeline> CreateComputePipeline() override;

		inline void SetRenderPass(GraphicsRenderPass* renderPass) override { _currentRenderPass = renderPass; }

		inline void ResetRenderPass() override {
			_currentRenderPass = _mainRenderPass.get();
			_mainRenderPass->Bind(false, false);
		}

		inline GraphicsRenderPass* GetRenderPass() const { return _currentRenderPass; }

		inline ComPtr<ID3D11Device> Device() const { return _device; }
		inline ComPtr<ID3D11DeviceContext> DeviceContext() const { return _deviceContext; }
		inline ComPtr<IDXGISwapChain> SwapChain() const { return _swapChain; }

	private:
		int32_t CreateSwapChain();
		int32_t CreateMainRenderPass();

		ID3D11SamplerState* CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v, D3D11_TEXTURE_ADDRESS_MODE w);

		void SetVSync(bool enable);

	private:
		HWND _hWnd;

		float _swapInterval;

		// rendering size
		int32_t _renderWidth, _renderHeight;

		ComPtr<ID3D11Device> _device;
		ComPtr<ID3D11DeviceContext> _deviceContext;

		ComPtr<IDXGISwapChain> _swapChain;

		Ref<GraphicsRenderPass> _mainRenderPass;
		GraphicsRenderPass* _currentRenderPass;

		Ref<Texture2D> _depthStencil;

		std::array<ID3D11SamplerState*, 2> _samplerStates;

		Ref<GraphicsCommandQueue> _commandQueue;
	};
}

#endif