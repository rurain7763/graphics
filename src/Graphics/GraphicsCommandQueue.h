#pragma once

#include "Core.h"
#include "GraphicsType.h"
#include "GraphicsBuffers.h"
#include "GraphicsPipelines.h"
#include "GraphicsTextures.h"
#include "GraphicsFramebuffer.h"

namespace flaw {
	class GraphicsCommandQueue {
	public:
		GraphicsCommandQueue() = default;
		virtual ~GraphicsCommandQueue() = default;

		virtual void SetPipelineBarrier(Ref<Texture> texture, TextureLayout oldLayout, TextureLayout newLayout, AccessTypes srcAccess, AccessTypes dstAccess, PipelineStages srcStage, PipelineStages dstStage) = 0;
		
		virtual void SetPipeline(const Ref<GraphicsPipeline>& pipeline) = 0;

		virtual void SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vertexBuffers) = 0;
		virtual void ResetVertexBuffers() = 0;
		virtual void SetShaderResources(const std::vector<Ref<ShaderResources>>& shaderResources) = 0;
		virtual void ResetShaderResources() = 0;

		virtual void Draw(uint32_t vertexCount, uint32_t vertexOffset = 0) = 0;
		virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0) = 0;
		virtual void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) = 0;
		virtual void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) = 0;
	
		virtual void BeginRenderPass() = 0;
		virtual void BeginRenderPass(const Ref<RenderPass>& beginRenderPass, const Ref<RenderPass>& resumeRenderPass, const Ref<Framebuffer>& framebuffer) = 0;
		virtual void EndRenderPass() = 0;

		virtual void Submit() = 0;
		
		virtual void Present() = 0;

		virtual uint32_t GetCurrentFrameIndex() const = 0;
		
		virtual void SetComputePipeline(const Ref<ComputePipeline>& pipeline) = 0;
		virtual void SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) = 0;
		virtual void SetComputeTexture(const Ref<Texture>& texture, TextureUsages texUsages, uint32_t slot) = 0;
		virtual void SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BufferUsages bufUsages, uint32_t slot) = 0;
		
		virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;
	};
}