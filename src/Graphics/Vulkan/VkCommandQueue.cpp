#include "pch.h"
#include "VkCommandQueue.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "VkSwapchain.h"
#include "VkPipelines.h"
#include "VkRenderPass.h"
#include "VkFramebuffer.h"
#include "VkPipelines.h"
#include "VkBuffers.h"
#include "VkShaders.h"
#include "VkTextures.h"

namespace flaw {
    VkCommandQueue::VkCommandQueue(VkContext& context)
        : _context(context)
    {
        if (!CreateCommandPools()) {
            Log::Fatal("Failed to create Vulkan command pools.");
            return;
        }

        if (!CreateCommandBuffers()) {
            Log::Fatal("Failed to create Vulkan command buffers.");
            return;
        }

        if (!SetupQueues()) {
            Log::Fatal("Failed to setup Vulkan queues.");
            return;
        }

        if (!CreateFences()) {
            Log::Fatal("Failed to create Vulkan fence.");
            return;
        }

        if (!CreateSemaphores()) {
            Log::Fatal("Failed to create Vulkan semaphores.");
            return;
        }

        _currentCommandBufferIndex = 0;
        _currentFrameIndex = 0;
        _currentFrameBuffers = _context.GetVkSwapchain().GetFramebuffers();

        Log::Info("Vulkan command queue initialized successfully.");
    }

    bool VkCommandQueue::CreateCommandPools() {
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = _context.GetGraphicsQueueFamilyIndex();

        auto result = _context.GetVkDevice().createCommandPool(poolInfo, nullptr, _context.GetVkDispatchLoader());
        if (result.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan command pool: %s", vk::to_string(result.result).c_str());
            return false;
        }

        _graphicsCommandPool = result.value;

        poolInfo.queueFamilyIndex = _context.GetTransferQueueFamilyIndex();

        result = _context.GetVkDevice().createCommandPool(poolInfo, nullptr, _context.GetVkDispatchLoader());
        if (result.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan command pool: %s", vk::to_string(result.result).c_str());
            return false;
        }

        _transferCommandPool = result.value;

        return true;
    }

    bool VkCommandQueue::CreateCommandBuffers() {
        auto& swapchain = _context.GetVkSwapchain();
        
        uint32_t frameBufferCount = swapchain.GetRenderTextureCount();

        vk::CommandBufferAllocateInfo mainCmdBuffAllocInfo;
        mainCmdBuffAllocInfo.commandPool = _graphicsCommandPool;
        mainCmdBuffAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        mainCmdBuffAllocInfo.commandBufferCount = 1;

        auto mainBuffWrapper = _context.GetVkDevice().allocateCommandBuffers(mainCmdBuffAllocInfo, _context.GetVkDispatchLoader());
        if (mainBuffWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to allocate Vulkan main command buffer: %s", vk::to_string(mainBuffWrapper.result).c_str());
            return false;
        }

        _graphicsMainCommandBuffer = mainBuffWrapper.value[0];

        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = _graphicsCommandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = frameBufferCount;
        
        auto buffWrapper = _context.GetVkDevice().allocateCommandBuffers(allocInfo, _context.GetVkDispatchLoader());
        if (buffWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to allocate Vulkan command buffers: %s", vk::to_string(buffWrapper.result).c_str());
            return false;
        }

        _graphicsFrameCommandBuffers = buffWrapper.value;

        vk::CommandBufferAllocateInfo transferAllocInfo;
        transferAllocInfo.commandPool = _transferCommandPool;
        transferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        transferAllocInfo.commandBufferCount = 1;

        auto transferBuffWrapper = _context.GetVkDevice().allocateCommandBuffers(transferAllocInfo, _context.GetVkDispatchLoader());
        if (transferBuffWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to allocate Vulkan transfer command buffer: %s", vk::to_string(transferBuffWrapper.result).c_str());
            return false;
        }

        _transferCommandBuffer = transferBuffWrapper.value[0];

        return true;
    }

    bool VkCommandQueue::SetupQueues() {
        _graphicsQueue = _context.GetVkDevice().getQueue(_context.GetGraphicsQueueFamilyIndex(), 0, _context.GetVkDispatchLoader());
        _presentQueue = _context.GetVkDevice().getQueue(_context.GetPresentQueueFamilyIndex(), 0, _context.GetVkDispatchLoader());
        _transferQueue = _context.GetVkDevice().getQueue(_context.GetTransferQueueFamilyIndex(), 0, _context.GetVkDispatchLoader());
        return true;
    }

    bool VkCommandQueue::CreateFences() {
        for (uint32_t i = 0; i < _graphicsFrameCommandBuffers.size(); ++i) {
            vk::FenceCreateInfo fenceInfo;
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

            auto fenceWrapper = _context.GetVkDevice().createFence(fenceInfo, nullptr, _context.GetVkDispatchLoader());
            if (fenceWrapper.result != vk::Result::eSuccess) {
                Log::Error("Failed to create Vulkan fence: %s", vk::to_string(fenceWrapper.result).c_str());
                return false;
            }

            _inFlightFences.push_back(fenceWrapper.value);
        }

        return true;
    }

    bool VkCommandQueue::CreateSemaphores() {
        for (uint32_t i = 0; i < _graphicsFrameCommandBuffers.size(); ++i) {
            vk::SemaphoreCreateInfo semaphoreInfo;

            auto semaphoreWrapper = _context.GetVkDevice().createSemaphore(semaphoreInfo, nullptr, _context.GetVkDispatchLoader());
            if (semaphoreWrapper.result != vk::Result::eSuccess) {
                Log::Error("Failed to create Vulkan semaphore: %s", vk::to_string(semaphoreWrapper.result).c_str());
                return false;
            }

            _presentCompleteSemaphores.push_back(semaphoreWrapper.value);

            semaphoreWrapper = _context.GetVkDevice().createSemaphore(semaphoreInfo, nullptr, _context.GetVkDispatchLoader());
            if (semaphoreWrapper.result != vk::Result::eSuccess) {
                Log::Error("Failed to create Vulkan semaphore: %s", vk::to_string(semaphoreWrapper.result).c_str());
                return false;
            }

            _renderCompleteSemaphores.push_back(semaphoreWrapper.value);
        }

        return true;
    }

    VkCommandQueue::~VkCommandQueue() {
        for (uint32_t i = 0; i < _graphicsFrameCommandBuffers.size(); ++i) {
            _context.GetVkDevice().destroySemaphore(_renderCompleteSemaphores[i], nullptr, _context.GetVkDispatchLoader());
            _context.GetVkDevice().destroyFence(_inFlightFences[i], nullptr, _context.GetVkDispatchLoader());
            _context.GetVkDevice().destroySemaphore(_presentCompleteSemaphores[i], nullptr, _context.GetVkDispatchLoader());
        }

        // NOTE: Because command buffers are allocated from a command pool, we don't need to destroy them individually.
        _context.GetVkDevice().destroyCommandPool(_transferCommandPool, nullptr, _context.GetVkDispatchLoader());
        _context.GetVkDevice().destroyCommandPool(_graphicsCommandPool, nullptr, _context.GetVkDispatchLoader());
    }

    bool VkCommandQueue::Prepare() {
        _context.GetVkDevice().waitForFences(1, &_inFlightFences[_currentCommandBufferIndex], VK_TRUE, UINT64_MAX);

        auto& swapchain = _context.GetVkSwapchain();

        auto acquireWrapper = _context.GetVkDevice().acquireNextImageKHR(swapchain.GetNativeVkSwapchain(), UINT64_MAX, _presentCompleteSemaphores[_currentCommandBufferIndex], nullptr);
        if (acquireWrapper.result == vk::Result::eSuboptimalKHR) {
            Log::Warn("Vulkan swapchain is in a suboptimal state: %s", vk::to_string(acquireWrapper.result).c_str());
            return false;
        } else if (acquireWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to acquire next image from Vulkan swapchain: %s", vk::to_string(acquireWrapper.result).c_str());
            return false;
        }

        _currentFrameIndex = acquireWrapper.value;
  
        // Begin command buffer
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        vk::CommandBufferBeginInfo beginInfo;
        auto result = commandBuffer.begin(beginInfo);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to begin Vulkan command buffer: %s", vk::to_string(result).c_str());
        }

        auto vkFramebuffer = _context.GetVkSwapchain().GetFramebuffer(_currentFrameIndex); 
        auto vkRenderPass = _context.GetVkSwapchain().GetClearOpRenderPass();

        std::array<vk::ClearValue, 2> clearValues = {
            vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}),
            vk::ClearDepthStencilValue(1.0f, 0)
        };

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = vkRenderPass->GetNativeVkRenderPass();
        renderPassInfo.framebuffer = vkFramebuffer->GetNativeVkFramebuffer();
        renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
        renderPassInfo.renderArea.extent = vk::Extent2D{ vkFramebuffer->GetWidth(), vkFramebuffer->GetHeight() };
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        
        return true;
    }

    void VkCommandQueue::Present() {
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        commandBuffer.endRenderPass();

        auto result = commandBuffer.end();
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to end Vulkan command buffer: %s", vk::to_string(result).c_str());
        }

        vk::Semaphore waitSemaphores[] = { _presentCompleteSemaphores[_currentCommandBufferIndex] };
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        vk::Semaphore signalSemaphores[] = { _renderCompleteSemaphores[_currentCommandBufferIndex] };

        vk::SubmitInfo submitInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        _context.GetVkDevice().resetFences(1, &_inFlightFences[_currentCommandBufferIndex]);
        result = _graphicsQueue.submit(1, &submitInfo, _inFlightFences[_currentCommandBufferIndex]);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to submit Vulkan command buffer: %s", vk::to_string(result).c_str());
        }

        vk::PresentInfoKHR presentInfo;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &_context.GetVkSwapchain().GetNativeVkSwapchain();
        presentInfo.pImageIndices = &_currentFrameIndex;

        result = _presentQueue.presentKHR(presentInfo);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to present Vulkan swapchain: %s", vk::to_string(result).c_str());
        }

        _currentCommandBufferIndex = (_currentCommandBufferIndex + 1) % _graphicsFrameCommandBuffers.size();
    }

    void VkCommandQueue::SetFramebuffers(const std::vector<Ref<GraphicsFramebuffer>>& framebuffers, const std::vector<Ref<GraphicsRenderPass>>& renderPasses) {
        _currentFrameBuffers.resize(framebuffers.size());
        for (size_t i = 0; i < framebuffers.size(); ++i) {
            auto vkFramebuffer = std::dynamic_pointer_cast<VkFramebuffer>(framebuffers[i]);
            FASSERT(vkFramebuffer, "Invalid framebuffer type for Vulkan command queue");
            auto vkRenderPass = std::dynamic_pointer_cast<VkRenderPass>(renderPasses[i]);
            FASSERT(vkRenderPass, "Invalid render pass type for Vulkan command queue");

            _currentFrameBuffers[i] = vkFramebuffer;
        }

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];
        auto vkFramebuffer = _currentFrameBuffers[_currentFrameIndex];
        auto vkRenderPass = std::static_pointer_cast<VkRenderPass>(renderPasses[_currentFrameIndex]);

        commandBuffer.endRenderPass();

        int32_t renderWidth, renderHeight;
        _context.GetSize(renderWidth, renderHeight);
        vkFramebuffer->Resize(renderWidth, renderHeight);

        std::vector<vk::ClearValue> clearValues;

        for (uint32_t i = 0; i < vkRenderPass->GetColorAttachmentCount(); ++i) {
            if (vkRenderPass->GetColorAttachmentLoadOp(i) == AttachmentLoadOp::Clear) {
                clearValues.push_back(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
            }
            else {
                clearValues.push_back({});
            }
        }

        if (vkRenderPass->HasDepthStencilAttachment()) {
            if (vkRenderPass->GetDepthStencilAttachmentLoadOp() == AttachmentLoadOp::Clear) {
                clearValues.push_back(vk::ClearDepthStencilValue(1.0f, 0));
            } else {
                clearValues.push_back({});
            }
        }

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = vkRenderPass->GetNativeVkRenderPass();
        renderPassInfo.framebuffer = vkFramebuffer->GetNativeVkFramebuffer();
        renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
        renderPassInfo.renderArea.extent = vk::Extent2D{ vkFramebuffer->GetWidth(), vkFramebuffer->GetHeight() };
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline); 
    }

    void VkCommandQueue::ResetFramebuffers() {        
        _currentFrameBuffers = _context.GetVkSwapchain().GetFramebuffers();

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];
        auto vkFramebuffer = _currentFrameBuffers[_currentFrameIndex];
        auto vkRenderPass = _context.GetVkSwapchain().GetLoadOpRenderPass();
        
        commandBuffer.endRenderPass();

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = vkRenderPass->GetNativeVkRenderPass();
        renderPassInfo.framebuffer = vkFramebuffer->GetNativeVkFramebuffer();
        renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
        renderPassInfo.renderArea.extent = vk::Extent2D{ vkFramebuffer->GetWidth(), vkFramebuffer->GetHeight() };
        renderPassInfo.clearValueCount = 0;
        renderPassInfo.pClearValues = nullptr;

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline); 
    }

    void VkCommandQueue::SetPipeline(const Ref<GraphicsPipeline>& pipeline) {
        auto vkPipeline = std::dynamic_pointer_cast<VkGraphicsPipeline>(pipeline);
        FASSERT(vkPipeline, "Invalid pipeline type for Vulkan command queue");
        
        _currentPipeline = vkPipeline->GetNativeVkGraphicsPipeline();
        _currentPipelineLayout = vkPipeline->GetVkPipelineLayout();
        _currentPushConstantRanges = vkPipeline->GetVkPushConstantRanges();
        _currentDescriptorSets.resize(vkPipeline->GetVkDescriptorSetLayouts().size());

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _currentPipeline);

        const uint32_t behaviorStates = vkPipeline->GetBehaviorStates();

        if (behaviorStates & GraphicsPipeline::BehaviorFlag::AutoResizeViewport) {
            auto framebuffer = _currentFrameBuffers[_currentFrameIndex];

            vk::Viewport viewport;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(framebuffer->GetWidth());
            viewport.height = static_cast<float>(framebuffer->GetHeight());
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            commandBuffer.setViewport(0, 1, &viewport);
        }

        if (behaviorStates & GraphicsPipeline::BehaviorFlag::AutoResizeScissor) {
            auto framebuffer = _currentFrameBuffers[_currentFrameIndex];

            vk::Rect2D scissor;
            scissor.offset = vk::Offset2D{ 0, 0 };
            scissor.extent = vk::Extent2D{ framebuffer->GetWidth(), framebuffer->GetHeight() };
            commandBuffer.setScissor(0, 1, &scissor);
        }
    }

    void VkCommandQueue::SetPipelinePushConstant(uint32_t rangeIndex, const void* data) {
        if (!_currentPipeline) {
            Log::Error("No pipeline set for the command queue.");
            return;
        }

        const auto& pushConstantRange = _currentPushConstantRanges.at(rangeIndex);

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];
        commandBuffer.pushConstants(_currentPipelineLayout, pushConstantRange.stageFlags, pushConstantRange.offset, pushConstantRange.size, data, _context.GetVkDispatchLoader());
    }

    void VkCommandQueue::SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
        auto vkVertexBuffer = std::dynamic_pointer_cast<VkVertexBuffer>(vertexBuffer);
        FASSERT(vkVertexBuffer, "Invalid vertex buffer type for Vulkan command queue");

        _currentVertexBuffer = vkVertexBuffer;
    }

    void VkCommandQueue::SetShaderResources(const Ref<ShaderResources>& shaderResources, uint32_t set) {
        auto vkShaderResources = std::dynamic_pointer_cast<VkShaderResources>(shaderResources);
        FASSERT(vkShaderResources, "Invalid shader resources type for Vulkan command queue");

        _currentDescriptorSets[set] = vkShaderResources->GetVkDescriptorSet();
    }

    void VkCommandQueue::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
        DrawInstanced(vertexCount, 1, vertexOffset);
    }

    void VkCommandQueue::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset) {
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        if (!_currentDescriptorSets.empty()) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _currentPipelineLayout, 0, _currentDescriptorSets.size(), _currentDescriptorSets.data(), 0, nullptr, _context.GetVkDispatchLoader());
        }

        if (_currentVertexBuffer) {
            vk::Buffer buffers[] = { _currentVertexBuffer->GetVkBuffer() };
            vk::DeviceSize offsets[] = { 0 };
    
            commandBuffer.bindVertexBuffers(0, 1, buffers, offsets, _context.GetVkDispatchLoader());
        }

        commandBuffer.draw(vertexCount, instanceCount, vertexOffset, 0, _context.GetVkDispatchLoader());
    }

    void VkCommandQueue::DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
        DrawIndexedInstanced(indexBuffer, indexCount, 1, indexOffset, vertexOffset);
    }

    void VkCommandQueue::DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset) {
        auto vkIndexBuffer = std::dynamic_pointer_cast<VkIndexBuffer>(indexBuffer);
        FASSERT(vkIndexBuffer, "Invalid index buffer type for Vulkan command queue");

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        if (!_currentDescriptorSets.empty()) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _currentPipelineLayout, 0, _currentDescriptorSets.size(), _currentDescriptorSets.data(), 0, nullptr, _context.GetVkDispatchLoader());
        }

        vk::Buffer vertexBuffers[] = { _currentVertexBuffer->GetVkBuffer() };
        vk::DeviceSize offsets[] = { 0 };
        commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets, _context.GetVkDispatchLoader());

        commandBuffer.bindIndexBuffer(vkIndexBuffer->GetVkBuffer(), 0, vk::IndexType::eUint32, _context.GetVkDispatchLoader());

        commandBuffer.drawIndexed(indexCount, instanceCount, indexOffset, vertexOffset, 0, _context.GetVkDispatchLoader());
    }

    void VkCommandQueue::SetComputePipeline(const Ref<ComputePipeline>& pipeline) {
        // Set the compute pipeline for the command queue
    }

    void VkCommandQueue::SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
        // Set the compute constant buffer for the command queue
    }

    void VkCommandQueue::SetComputeTexture(const Ref<Texture>& texture, BindFlag bindFlag, uint32_t slot) {
        // Set the compute texture for the command
    }

    void VkCommandQueue::SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) {
        // Set the compute structured buffer for the command queue
    }

    void VkCommandQueue::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
        // Issue a compute dispatch call with the specified dimensions
    }

    void VkCommandQueue::CopyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset) {
        _transferCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources, _context.GetVkDispatchLoader());

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        _transferCommandBuffer.begin(beginInfo, _context.GetVkDispatchLoader());

        vk::BufferCopy copyRegion;
        copyRegion.size = size;
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;

        _transferCommandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion, _context.GetVkDispatchLoader());

        _transferCommandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_transferCommandBuffer;

        auto result = _transferQueue.submit(1, &submitInfo, nullptr, _context.GetVkDispatchLoader());
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to submit Vulkan command buffer for copy: %s", vk::to_string(result).c_str());
            return;
        }

        _transferQueue.waitIdle(_context.GetVkDispatchLoader());
    }

    void VkCommandQueue::CopyBuffer(const vk::Buffer& srcBuffer, const vk::Image& dstImage, uint32_t width, uint32_t height, uint32_t srcOffset, uint32_t dstOffset, uint32_t arrayLayer) {
        _transferCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        _transferCommandBuffer.begin(beginInfo);

        vk::BufferImageCopy copyRegion;
        copyRegion.bufferOffset = srcOffset;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = arrayLayer;
        copyRegion.imageOffset = vk::Offset3D{ 0, 0, 0 };
        copyRegion.imageExtent = vk::Extent3D{ width, height, 1 };

        _transferCommandBuffer.copyBufferToImage(srcBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);

        _transferCommandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_transferCommandBuffer;

        auto result = _transferQueue.submit(1, &submitInfo, nullptr);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to submit Vulkan command buffer for copy: %s", vk::to_string(result).c_str());
            return;
        }

        _transferQueue.waitIdle();
    }

    void VkCommandQueue::CopyBuffer(const Ref<VertexBuffer>& srcBuffer, const Ref<VertexBuffer>& dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset) {
        auto vkSrcBuffer = std::dynamic_pointer_cast<VkVertexBuffer>(srcBuffer);
        auto vkDstBuffer = std::dynamic_pointer_cast<VkVertexBuffer>(dstBuffer);
        FASSERT(vkSrcBuffer && vkDstBuffer, "Invalid vertex buffer type for Vulkan command queue");

        CopyBuffer(vkSrcBuffer->GetVkBuffer(), vkDstBuffer->GetVkBuffer(), size, srcOffset, dstOffset);
    }

    void VkCommandQueue::TransitionImageLayout(const vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t arrayLayer) {
        _graphicsMainCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        _graphicsMainCommandBuffer.begin(beginInfo);

        vk::ImageMemoryBarrier barrier;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = arrayLayer;

        vk::PipelineStageFlags srcStageMask, dstStageMask;
        if (oldLayout == vk::ImageLayout::eUndefined) {
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
            dstStageMask = vk::PipelineStageFlagBits::eTransfer;
        }
        else {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            srcStageMask = vk::PipelineStageFlagBits::eTransfer;
            dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
        }

        _graphicsMainCommandBuffer.pipelineBarrier(srcStageMask, dstStageMask, vk::DependencyFlags(), nullptr, nullptr, barrier);

        _graphicsMainCommandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_graphicsMainCommandBuffer;

        auto result = _graphicsQueue.submit(1, &submitInfo, nullptr);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to submit Vulkan command buffer for copy: %s", vk::to_string(result).c_str());
            return;
        }
        
        _graphicsQueue.waitIdle();
    }

    void VkCommandQueue::Execute() {
    }
}

#endif