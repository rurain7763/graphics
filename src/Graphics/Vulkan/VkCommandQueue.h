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

        void SetPipeline(const Ref<GraphicsPipeline>& pipeline) override;
        void SetPipelinePushConstant(uint32_t rangeIndex, const void* data);

        void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
        void SetShaderResources(const Ref<ShaderResources>& shaderResources, uint32_t set = 0) override;

        void Draw(uint32_t vertexCount, uint32_t vertexOffset = 0) override;
        void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0) override;
        void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;
        void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;

        void BeginRenderPass() override;
        void BeginRenderPass(const Ref<GraphicsRenderPass>& beginRenderPass, const Ref<GraphicsRenderPass>& resumeRenderPass, const Ref<GraphicsFramebuffer>& framebuffer) override;
        void EndRenderPass() override;
        void Submit() override;

        void Present() override;

        uint32_t GetCurrentFrameIndex() const override { return _currentFrameIndex; }

        void SetComputePipeline(const Ref<ComputePipeline>& pipeline) override;
        void SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) override;
        void SetComputeTexture(const Ref<Texture>& texture, TextureUsage bindFlag, uint32_t slot) override;
        void SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, TextureUsage bindFlag, uint32_t slot) override;
        void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;

        void BeginOneTimeCommands();
        void EndOneTimeCommands();

        void CopyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset);
        void CopyBuffer(const vk::Buffer& srcBuffer, const vk::Image& dstImage, uint32_t width, uint32_t height, uint32_t srcOffset, uint32_t dstOffset, uint32_t arrayLayer = 1);
        void CopyBuffer(const Ref<VertexBuffer>& srcBuffer, const Ref<VertexBuffer>& dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset);

        void TransitionImageLayout( const vk::Image& image, 
                                    vk::ImageAspectFlags aspectMask,
                                    vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                    vk::AccessFlags srcAccessMask,
                                    vk::AccessFlags dstAccessMask,
                                    vk::PipelineStageFlags srcStageMask,
                                    vk::PipelineStageFlags dstStageMask );

        void GenerateMipmaps( const vk::Image& image, 
                              vk::ImageAspectFlags aspectMask, 
                              vk::Format format, uint32_t width, uint32_t height, 
                              uint32_t arrayLayer, 
                              uint32_t mipLevels,
                              vk::ImageLayout& oldLayout,
			                  vk::AccessFlags& srcAccessMask,
                              vk::PipelineStageFlags& srcStageMask);

    private:
        bool CreateCommandBuffers();
        bool CreateFences();
        bool CreateSemaphores();

        void BeginRenderPassImpl(const Ref<VkRenderPass>& renderPass, const Ref<VkFramebuffer>& framebuffer);

    private:
        VkContext& _context;

        std::vector<vk::CommandBuffer> _graphicsFrameCommandBuffers;
        std::vector<vk::Fence> _inFlightFences;
        std::vector<vk::Semaphore> _presentCompleteSemaphores;
        std::vector<vk::Semaphore> _renderCompleteSemaphores;

        struct BeginInfo {
            Ref<VkFramebuffer> framebuffer;
            Ref<VkRenderPass> beginRenderPass;
            Ref<VkRenderPass> resumeRenderPass;
        };

        uint32_t _currentCommandBufferIndex;
        uint32_t _currentFrameIndex;
        std::vector<BeginInfo> _currentBeginInfoStack;
        vk::Viewport _currentViewport;
        vk::Rect2D _currentScissor;
        vk::Pipeline _currentPipeline;
        vk::PipelineLayout _currentPipelineLayout;
        std::vector<vk::PushConstantRange> _currentPushConstantRanges;
        Ref<VkVertexBuffer> _currentVertexBuffer;
        std::vector<vk::DescriptorSet> _currentDescriptorSets;

        vk::Queue _oneTimeCommandQueue;
        vk::CommandBuffer _oneTimeCommandBuffer;
    };
}

#endif