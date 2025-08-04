#pragma once

#include "Core.h"
#include "GraphicsType.h"
#include "GraphicsBuffers.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"
#include "GraphicsTextures.h"
#include "GraphicsFramebuffer.h"

namespace flaw {
	class GraphicsCommandQueue {
	public:
		GraphicsCommandQueue() = default;
		virtual ~GraphicsCommandQueue() = default;

		virtual bool Prepare() = 0;
		virtual void Present() = 0;

		virtual void SetFramebuffers(const std::vector<Ref<GraphicsFramebuffer>>& framebuffers, const std::vector<Ref<GraphicsRenderPass>>& renderPasses) = 0;
		virtual void ResetFramebuffers() = 0;

		virtual void SetPipeline(const Ref<GraphicsPipeline>& pipeline) = 0;
		virtual void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void SetShaderResources(const Ref<ShaderResources>& shaderResources, uint32_t set = 0) = 0;

		virtual void Draw(uint32_t vertexCount, uint32_t vertexOffset = 0) = 0;
		virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0) = 0;
		virtual void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) = 0;
		virtual void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) = 0;
		
		virtual uint32_t GetCurrentFrameIndex() const = 0;

		virtual void SetComputePipeline(const Ref<ComputePipeline>& pipeline) = 0;
		virtual void SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) = 0;
		virtual void SetComputeTexture(const Ref<Texture>& texture, BindFlag bindFlag, uint32_t slot) = 0;
		virtual void SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) = 0;
		virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;

		virtual void Execute() = 0;
	};
}