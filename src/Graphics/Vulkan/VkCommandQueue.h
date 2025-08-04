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

        bool Prepare() override;
        void Present() override;

        void SetFramebuffers(const std::vector<Ref<GraphicsFramebuffer>>& framebuffers, const std::vector<Ref<GraphicsRenderPass>>& renderPasses) override;
        void ResetFramebuffers() override;

        void SetPipeline(const Ref<GraphicsPipeline>& pipeline) override;
        void SetPipelinePushConstant(uint32_t rangeIndex, const void* data);

        void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
        void SetShaderResources(const Ref<ShaderResources>& shaderResources, uint32_t set = 0) override;

        void Draw(uint32_t vertexCount, uint32_t vertexOffset = 0) override;
        void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0) override;
        void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;
        void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;

        uint32_t GetCurrentFrameIndex() const override { return _currentFrameIndex; }

        void SetComputePipeline(const Ref<ComputePipeline>& pipeline) override;
        void SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) override;
        void SetComputeTexture(const Ref<Texture>& texture, BindFlag bindFlag, uint32_t slot) override;
        void SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) override;
        void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;

        void CopyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset);
        void CopyBuffer(const vk::Buffer& srcBuffer, const vk::Image& dstImage, uint32_t width, uint32_t height, uint32_t srcOffset, uint32_t dstOffset, uint32_t arrayLayer = 1);
        void CopyBuffer(const Ref<VertexBuffer>& srcBuffer, const Ref<VertexBuffer>& dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset);

        void TransitionImageLayout(const vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t arrayLayer = 1);

        void Execute() override;

    private:
        bool CreateCommandPools();
        bool CreateCommandBuffers();
        bool SetupQueues();
        bool CreateFences();
        bool CreateSemaphores();

    private:
        VkContext& _context;

        vk::CommandPool _graphicsCommandPool;
        vk::CommandBuffer _graphicsMainCommandBuffer;
        std::vector<vk::CommandBuffer> _graphicsFrameCommandBuffers;
        std::vector<vk::Fence> _inFlightFences;
        std::vector<vk::Semaphore> _presentCompleteSemaphores;
        std::vector<vk::Semaphore> _renderCompleteSemaphores;

        vk::CommandPool _transferCommandPool;
        vk::CommandBuffer _transferCommandBuffer;

        vk::Queue _graphicsQueue;
        vk::Queue _presentQueue;
        vk::Queue _transferQueue;

        uint32_t _currentCommandBufferIndex;
        uint32_t _currentFrameIndex;
        std::vector<Ref<VkFramebuffer>> _currentFrameBuffers;
        vk::Pipeline _currentPipeline;
        vk::PipelineLayout _currentPipelineLayout;
        std::vector<vk::PushConstantRange> _currentPushConstantRanges;
        Ref<VkVertexBuffer> _currentVertexBuffer;
        std::vector<vk::DescriptorSet> _currentDescriptorSets;
    };
}

#endif