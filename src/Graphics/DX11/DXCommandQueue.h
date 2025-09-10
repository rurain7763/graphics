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

		void SetPipelineBarrier(Ref<VertexBuffer> buffer, AccessTypes srcAccess, AccessTypes dstAccess, PipelineStages srcStage, PipelineStages dstStage) override;
		void SetPipelineBarrier(const std::vector<Ref<Texture>>& textures, TextureLayout oldLayout, TextureLayout newLayout, AccessTypes srcAccess, AccessTypes dstAccess, PipelineStages srcStage, PipelineStages dstStage) override;

		void SetPipeline(const Ref<GraphicsPipeline>& pipeline) override;

		void SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vertexBuffers) override;
		void ResetVertexBuffers() override;
		void SetShaderResources(const std::vector<Ref<ShaderResources>>& shaderResources) override;
		void ResetShaderResources() override;

		void Draw(uint32_t vertexCount, uint32_t vertexOffset = 0) override;
		void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0, uint32_t instanceOffset = 0) override;
		void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;
		void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0, uint32_t instanceOffset = 0) override;
		
		void BeginRenderPass(const Ref<RenderPass>& renderpass, const Ref<Framebuffer>& framebuffer) override;
		void NextSubpass() override;
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
		void BindComputeResources();

	private:
		DXContext& _context;

		std::vector<Ref<DXVertexBuffer>> _currentVertexBuffers;
		std::vector<ID3D11Buffer*> _currentDXVertexBuffers;
		std::vector<UINT> _currentDXVertexBufferStrides;
		std::vector<UINT> _currentDXVertexBufferOffsets;

		Ref<DXGraphicsPipeline> _currentGraphicsPipeline;

		std::vector<Ref<DXShaderResources>> _currentShaderResourcesVec;

		Ref<DXRenderPass> _currentBeginRenderPass;
		uint32_t _currentBeginSubpass;
		Ref<DXFramebuffer> _currentBeginFramebuffer;

		D3D11_VIEWPORT _currentViewport;
		D3D11_RECT _currentScissor;

		bool _needBindComputePipeline = false;
		Ref<DXComputePipeline> _currentComputePipeline;
	};
}

#endif