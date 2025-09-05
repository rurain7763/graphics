#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsCommandQueue.h"

namespace flaw {
    class VkContext;
    class VkGraphicsPipeline;
    class VkRenderPass;
    class VkFramebuffer;
    class VkVertexBuffer;

    class VkCommandQueue : public GraphicsCommandQueue {
    public:
        VkCommandQueue(VkContext& context);
        ~VkCommandQueue() override;

        bool Prepare();

		void SetPipelineBarrier(Ref<Texture> texture, TextureLayout oldLayout, TextureLayout newLayout, AccessTypes srcAccess, AccessTypes dstAccess, PipelineStages srcStage, PipelineStages dstStage) override;

        void SetPipeline(const Ref<GraphicsPipeline>& pipeline) override;
        void SetPushConstants(uint32_t rangeIndex, const void* data);

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

        uint32_t GetCurrentFrameIndex() const override { return _currentFrameIndex; }

        void SetComputePipeline(const Ref<ComputePipeline>& pipeline) override;
        void SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) override;
        void SetComputeTexture(const Ref<Texture>& texture, TextureUsages texUsages, uint32_t slot) override;
        void SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BufferUsages bufUsages, uint32_t slot) override;
        void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;

        vk::CommandBuffer BeginOneTimeCommands();
        void EndOneTimeCommands(vk::CommandBuffer commandBuffer);

    private:
        bool CreateCommandBuffers();
        bool CreateFences();
        bool CreateSemaphores();

    private:
        VkContext& _context;

        std::vector<vk::CommandBuffer> _graphicsFrameCommandBuffers;
        std::vector<vk::Fence> _inFlightFences;
        std::vector<vk::Semaphore> _presentCompleteSemaphores;
        std::vector<vk::Semaphore> _renderCompleteSemaphores;

        uint32_t _currentCommandBufferIndex;
        uint32_t _currentFrameIndex;
		Ref<VkRenderPass> _currentBeginRenderPass;
		Ref<VkFramebuffer> _currentBeginFramebuffer;
		Ref<VkGraphicsPipeline> _currentPipeline;
    };
}

#endif