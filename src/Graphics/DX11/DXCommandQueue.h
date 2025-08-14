#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsCommandQueue.h"

namespace flaw {
	class DXContext;
	class DXVertexBuffer;
	class DXGraphicsPipeline;
	class DXShaderResources;
	class DXFramebuffer;
	class DXRenderPass;
	class DXComputePipeline;

	class DXCommandQueue : public GraphicsCommandQueue {
	public:
		DXCommandQueue(DXContext& context);
		virtual ~DXCommandQueue() = default;

		void SetPipeline(const Ref<GraphicsPipeline>& pipeline) override;

		void SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vertexBuffers) override;
		void SetShaderResources(const std::vector<Ref<ShaderResources>>& shaderResources) override;

		void Draw(uint32_t vertexCount, uint32_t vertexOffset = 0) override;
		void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0) override;
		void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;
		void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;
		
		void BeginRenderPass() override;
		void BeginRenderPass(const Ref<GraphicsRenderPass>& beginRenderPass, const Ref<GraphicsRenderPass>& resumeRenderPass, const Ref<GraphicsFramebuffer>& framebuffer) override;
		void EndRenderPass() override;

		void Submit() override;

		void Present() override;

		uint32_t GetCurrentFrameIndex() const override {
			return 0; // DX11 does not have frame indices like Vulkan or DirectX 12
		}

		void SetComputePipeline(const Ref<ComputePipeline>& pipeline) override;
		void SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) override;
		void SetComputeTexture(const Ref<Texture>& texture, TextureUsages texUsages, uint32_t slot) override;
		void SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BufferUsages buffUsages, uint32_t slot) override;
		
		void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;

	private:
		void BindRenderResources();
		void BeginRenderPassImpl(const Ref<DXRenderPass>& beginRenderPass, const Ref<DXFramebuffer>& framebuffer);

		void BindComputeResources();

	private:
		DXContext& _context;

		bool _needBindVertexBuffers;
		std::vector<Ref<DXVertexBuffer>> _currentVertexBuffers;
		std::vector<ID3D11Buffer*> _currentDXVertexBuffers;
		std::vector<UINT> _currentDXVertexBufferStrides;
		std::vector<UINT> _currentDXVertexBufferOffsets;

		bool _needBindGraphicsPipeline;
		Ref<DXGraphicsPipeline> _currentGraphicsPipeline;

		bool _needBindShaderResources;
		Ref<DXShaderResources> _currentShaderResources;

		struct BeginInfo {
			Ref<DXFramebuffer> framebuffer;
			Ref<DXRenderPass> beginRenderPass;
			Ref<DXRenderPass> resumeRenderPass;
		};
		std::vector<BeginInfo> _currentBeginInfoStack;

		D3D11_VIEWPORT _currentViewport;
		D3D11_RECT _currentScissor;

		bool _needBindComputePipeline = false;
		Ref<DXComputePipeline> _currentComputePipeline;
	};
}

#endif