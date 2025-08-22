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
        if (!CreateCommandBuffers()) {
            Log::Fatal("Failed to create Vulkan command buffers.");
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
		_currentPipeline = nullptr;
		_currentPipelineLayout = nullptr;
		_currentPushConstantRanges.clear();
		_needBindVertexBuffers = false;
		_currentVertexBuffers.clear();
		_currentVertexBufferOffsets.clear();
		_needBindDescriptorSets = false;
		_currentDescriptorSets.clear();

        Log::Info("Vulkan command queue initialized successfully.");
    }

    bool VkCommandQueue::CreateCommandBuffers() {
        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = _context.GetVkGraphicsCommandPool();
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = _context.GetFrameCount();
        
        auto buffWrapper = _context.GetVkDevice().allocateCommandBuffers(allocInfo);
        if (buffWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to allocate Vulkan command buffers: %s", vk::to_string(buffWrapper.result).c_str());
            return false;
        }

        _graphicsFrameCommandBuffers = buffWrapper.value;

        return true;
    }

    bool VkCommandQueue::CreateFences() {
        for (uint32_t i = 0; i < _graphicsFrameCommandBuffers.size(); ++i) {
            vk::FenceCreateInfo fenceInfo;
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

            auto fenceWrapper = _context.GetVkDevice().createFence(fenceInfo, nullptr);
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

            auto semaphoreWrapper = _context.GetVkDevice().createSemaphore(semaphoreInfo, nullptr);
            if (semaphoreWrapper.result != vk::Result::eSuccess) {
                Log::Error("Failed to create Vulkan semaphore: %s", vk::to_string(semaphoreWrapper.result).c_str());
                return false;
            }

            _presentCompleteSemaphores.push_back(semaphoreWrapper.value);

            semaphoreWrapper = _context.GetVkDevice().createSemaphore(semaphoreInfo, nullptr);
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
            _context.GetVkDevice().destroySemaphore(_renderCompleteSemaphores[i], nullptr);
            _context.GetVkDevice().destroyFence(_inFlightFences[i], nullptr);
            _context.GetVkDevice().destroySemaphore(_presentCompleteSemaphores[i], nullptr);
        }
    }

    bool VkCommandQueue::Prepare() {
        _context.GetVkDevice().waitForFences(1, &_inFlightFences[_currentCommandBufferIndex], VK_TRUE, UINT64_MAX);

        auto acquireWrapper = _context.GetVkDevice().acquireNextImageKHR(_context.GetVkSwapchain().GetNativeVkSwapchain(), UINT64_MAX, _presentCompleteSemaphores[_currentCommandBufferIndex], nullptr);
        if (acquireWrapper.result == vk::Result::eSuboptimalKHR) {
            LOG_WARN("Vulkan swapchain is in a suboptimal state");
        }
        else if (acquireWrapper.result == vk::Result::eErrorOutOfDateKHR) {
            LOG_ERROR("Vulkan swapchain is out of date: %s", vk::to_string(acquireWrapper.result).c_str());
            return false;
        } 
        else if (acquireWrapper.result != vk::Result::eSuccess) {
            LOG_FATAL("Failed to acquire next image from Vulkan swapchain: %s", vk::to_string(acquireWrapper.result).c_str());
            return false;
        }

        _context.GetVkDevice().resetFences(1, &_inFlightFences[_currentCommandBufferIndex]);

        _currentFrameIndex = acquireWrapper.value;

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        vk::CommandBufferBeginInfo beginInfo;
        auto result = commandBuffer.begin(beginInfo);
        if (result != vk::Result::eSuccess) {
            LOG_FATAL("Failed to begin Vulkan command buffer: %s", vk::to_string(result).c_str());
            return false;
        }
  
        return true;
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

        if (behaviorStates & GraphicsPipeline::Behavior::AutoResizeViewport) {
            commandBuffer.setViewport(0, 1, &_currentViewport);
        }

        if (behaviorStates & GraphicsPipeline::Behavior::AutoResizeScissor) {
            commandBuffer.setScissor(0, 1, &_currentScissor);
        }
    }

    void VkCommandQueue::SetPipelinePushConstant(uint32_t rangeIndex, const void* data) {
        if (!_currentPipeline) {
            Log::Error("No pipeline set for the command queue.");
            return;
        }

        const auto& pushConstantRange = _currentPushConstantRanges.at(rangeIndex);

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];
        commandBuffer.pushConstants(_currentPipelineLayout, pushConstantRange.stageFlags, pushConstantRange.offset, pushConstantRange.size, data);
    }

    void VkCommandQueue::SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vertexBuffers) {
		_needBindVertexBuffers = true;

		_currentVertexBuffers.resize(vertexBuffers.size());
		_currentVertexBufferOffsets.resize(vertexBuffers.size());
        for (uint32_t i = 0; i < vertexBuffers.size(); i++) {
			auto vkVertexBuffer = std::static_pointer_cast<VkVertexBuffer>(vertexBuffers[i]);
			FASSERT(vkVertexBuffer, "Invalid vertex buffer type for Vulkan command queue");

			_currentVertexBuffers[i] = vkVertexBuffer->GetVkBuffer();
			_currentVertexBufferOffsets[i] = 0; // Assuming no offset for simplicity
		}
    }

    void VkCommandQueue::ResetVertexBuffers() {
        _needBindVertexBuffers = true;

		_currentVertexBuffers.clear();
		_currentVertexBufferOffsets.clear();
    }

    void VkCommandQueue::SetShaderResources(const std::vector<Ref<ShaderResources>>& shaderResources) {
		_needBindDescriptorSets = true;

		_currentDescriptorSets.resize(shaderResources.size());
		for (uint32_t i = 0; i < shaderResources.size(); ++i) {
            auto vkShaderResources = std::static_pointer_cast<VkShaderResources>(shaderResources[i]);
            FASSERT(vkShaderResources, "Invalid shader resources type for Vulkan command queue");

            _currentDescriptorSets[i] = vkShaderResources->GetVkDescriptorSet();
		}
    }

    void VkCommandQueue::ResetShaderResources() {
        _needBindDescriptorSets = true;
        _currentDescriptorSets.clear();
    }

    void VkCommandQueue::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
        DrawInstanced(vertexCount, 1, vertexOffset);
    }

    void VkCommandQueue::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset) {
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        if (_needBindDescriptorSets) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _currentPipelineLayout, 0, _currentDescriptorSets.size(), _currentDescriptorSets.data(), 0, nullptr);
			_needBindDescriptorSets = false;
        }

        if (_needBindVertexBuffers) {
            if (!_currentVertexBuffers.empty()) {
		        commandBuffer.bindVertexBuffers(0, _currentVertexBuffers.size(), _currentVertexBuffers.data(), _currentVertexBufferOffsets.data());
            }
			_needBindVertexBuffers = false;
        }

        commandBuffer.draw(vertexCount, instanceCount, vertexOffset, 0);
    }

    void VkCommandQueue::DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
        DrawIndexedInstanced(indexBuffer, indexCount, 1, indexOffset, vertexOffset);
    }

    void VkCommandQueue::DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset) {
        auto vkIndexBuffer = std::dynamic_pointer_cast<VkIndexBuffer>(indexBuffer);
        FASSERT(vkIndexBuffer, "Invalid index buffer type for Vulkan command queue");

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        if (_needBindDescriptorSets) {                
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _currentPipelineLayout, 0, _currentDescriptorSets.size(), _currentDescriptorSets.data(), 0, nullptr);
			_needBindDescriptorSets = false;
        }

        if (_needBindVertexBuffers) {
            if (!_currentVertexBuffers.empty()) {
                commandBuffer.bindVertexBuffers(0, _currentVertexBuffers.size(), _currentVertexBuffers.data(), _currentVertexBufferOffsets.data());
            }
            _needBindVertexBuffers = false;
        }

        commandBuffer.bindIndexBuffer(vkIndexBuffer->GetVkBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(indexCount, instanceCount, indexOffset, vertexOffset, 0);
    }

    void VkCommandQueue::BeginRenderPassImpl(const Ref<VkRenderPass>& renderPass, const Ref<VkFramebuffer>& framebuffer) {
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        std::vector<vk::ClearValue> clearValues;
        for(uint32_t i = 0; i < renderPass->GetColorAttachmentOpCount(); ++i) {
            if (renderPass->GetColorAttachmentOp(i).loadOp == AttachmentLoadOp::Clear) {
                clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
            } else {
                clearValues.push_back(vk::ClearColorValue());
            }
        }

        if (renderPass->HasDepthStencilAttachmentOp()) {
            if (renderPass->GetDepthStencilAttachmentOp().loadOp == AttachmentLoadOp::Clear) {
                clearValues.push_back(vk::ClearDepthStencilValue(1.0f, 0));
            } else {
                clearValues.push_back(vk::ClearDepthStencilValue());
            }
        }

        if (renderPass->HasResolveAttachmentOp()) {
            if (renderPass->GetResolveAttachmentOp().loadOp == AttachmentLoadOp::Clear) {
                clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
            } else {
                clearValues.push_back(vk::ClearColorValue());
            }
        }

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass->GetNativeVkRenderPass();
        renderPassInfo.framebuffer = framebuffer->GetNativeVkFramebuffer();
        renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
        renderPassInfo.renderArea.extent = vk::Extent2D{ framebuffer->GetWidth(), framebuffer->GetHeight() };
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    }

    void VkCommandQueue::BeginRenderPass() {        
        auto& swapchain = _context.GetVkSwapchain();
        auto vkFramebuffer = swapchain.GetFramebuffer(_currentFrameIndex); 

        BeginRenderPass(swapchain.GetClearOpRenderPass(), swapchain.GetLoadOpRenderPass(), vkFramebuffer);
    }

    void VkCommandQueue::BeginRenderPass(const Ref<RenderPass>& beginRenderPass, const Ref<RenderPass>& resumeRenderPass, const Ref<GraphicsFramebuffer>& framebuffer) {
        auto vkBeginRenderPass = std::static_pointer_cast<VkRenderPass>(beginRenderPass);
        FASSERT(vkBeginRenderPass, "Invalid render pass type for Vulkan command queue");

        auto vkResumeRenderPass = std::static_pointer_cast<VkRenderPass>(resumeRenderPass);
        FASSERT(vkResumeRenderPass, "Invalid render pass type for Vulkan command queue");

        auto vkFramebuffer = std::static_pointer_cast<VkFramebuffer>(framebuffer);
        FASSERT(vkFramebuffer, "Invalid framebuffer type for Vulkan command queue");

        if (!_currentBeginInfoStack.empty()) {
            auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];
            commandBuffer.endRenderPass();
        }
        
        _currentViewport.x = 0.0f;
        _currentViewport.y = 0.0f;
        _currentViewport.width = static_cast<float>(vkFramebuffer->GetWidth());
        _currentViewport.height = static_cast<float>(vkFramebuffer->GetHeight());
        _currentViewport.minDepth = 0.0f;
        _currentViewport.maxDepth = 1.0f;

        _currentScissor.offset = vk::Offset2D{ 0, 0 };
        _currentScissor.extent = vk::Extent2D{ vkFramebuffer->GetWidth(), vkFramebuffer->GetHeight() };

        _currentBeginInfoStack.push_back({ vkFramebuffer, vkBeginRenderPass, vkResumeRenderPass });

        BeginRenderPassImpl(vkBeginRenderPass, vkFramebuffer);
    }

    void VkCommandQueue::EndRenderPass() {
        if (_currentBeginInfoStack.empty()) {
            Log::Error("No render pass is currently active.");
            return;
        }

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        commandBuffer.endRenderPass();

        _currentBeginInfoStack.pop_back();
        if (_currentBeginInfoStack.empty()) {
            return;
        }

        auto& currentBeginInfo = _currentBeginInfoStack.back();
        auto vkFramebuffer = currentBeginInfo.framebuffer;
        auto vkRenderPass = currentBeginInfo.resumeRenderPass;

        _currentViewport.x = 0.0f;
        _currentViewport.y = 0.0f;
        _currentViewport.width = static_cast<float>(vkFramebuffer->GetWidth());
        _currentViewport.height = static_cast<float>(vkFramebuffer->GetHeight());
        _currentViewport.minDepth = 0.0f;
        _currentViewport.maxDepth = 1.0f;

        _currentScissor.offset = vk::Offset2D{ 0, 0 };
        _currentScissor.extent = vk::Extent2D{ vkFramebuffer->GetWidth(), vkFramebuffer->GetHeight() };

        BeginRenderPassImpl(vkRenderPass, vkFramebuffer);
    }

    void VkCommandQueue::Submit() {
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        auto result = commandBuffer.end();
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to end Vulkan command buffer: %s", vk::to_string(result).c_str());
            return;
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

        result = _context.GetVkGraphicsQueue().submit(1, &submitInfo, _inFlightFences[_currentCommandBufferIndex]);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to submit Vulkan command buffer: %s", vk::to_string(result).c_str());
            return;
        }
    }

    void VkCommandQueue::Present() {
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        vk::Semaphore signalSemaphores[] = { _renderCompleteSemaphores[_currentCommandBufferIndex] };

        vk::PresentInfoKHR presentInfo;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &_context.GetVkSwapchain().GetNativeVkSwapchain();
        presentInfo.pImageIndices = &_currentFrameIndex;

        auto result = _context.GetVkPresentQueue().presentKHR(presentInfo);
        if (result == vk::Result::eSuboptimalKHR) {
            LOG_WARN("Vulkan swapchain is in a suboptimal state");
        } 
        else if (result == vk::Result::eErrorOutOfDateKHR) {
            LOG_ERROR("Vulkan swapchain is out of date: %s", vk::to_string(result).c_str());
        }
        else if (result != vk::Result::eSuccess) {
            LOG_FATAL("Failed to present Vulkan swapchain: %s", vk::to_string(result).c_str());
        }

        _currentCommandBufferIndex = (_currentCommandBufferIndex + 1) % _graphicsFrameCommandBuffers.size();
    }

    void VkCommandQueue::SetComputePipeline(const Ref<ComputePipeline>& pipeline) {
        // Set the compute pipeline for the command queue
    }

    void VkCommandQueue::SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
        // Set the compute constant buffer for the command queue
    }

    void VkCommandQueue::SetComputeTexture(const Ref<Texture>& texture, TextureUsages texUsages, uint32_t slot) {
        // Set the compute texture for the command
    }

    void VkCommandQueue::SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BufferUsages bufUsages, uint32_t slot) {
        // Set the compute structured buffer for the command queue
    }

    void VkCommandQueue::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
        // Issue a compute dispatch call with the specified dimensions
    }

    void VkCommandQueue::BeginOneTimeCommands() {
        auto device = _context.GetVkDevice();
        auto graphicsQueue = _context.GetVkGraphicsQueue();
        auto graphicsCommandPool = _context.GetVkGraphicsCommandPool();

        vk::CommandBufferAllocateInfo buffAllocInfo;
        buffAllocInfo.commandPool = graphicsCommandPool;
        buffAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        buffAllocInfo.commandBufferCount = 1;

        auto buffWrapper = device.allocateCommandBuffers(buffAllocInfo);
        if (buffWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to allocate one-time command buffer: %s", vk::to_string(buffWrapper.result).c_str());
            return;
        }

        _oneTimeCommandBuffer = buffWrapper.value[0];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        auto result = _oneTimeCommandBuffer.begin(beginInfo);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to begin one-time command buffer: %s", vk::to_string(result).c_str());
            return;
        }

        _oneTimeCommandQueue = graphicsQueue;
    }

    void VkCommandQueue::EndOneTimeCommands() {
        if (!_oneTimeCommandBuffer) {
            Log::Error("No one-time command buffer is currently active.");
            return;
        }

        _oneTimeCommandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_oneTimeCommandBuffer;

        auto result = _oneTimeCommandQueue.submit(1, &submitInfo, nullptr);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to submit one-time command buffer: %s", vk::to_string(result).c_str());
            return;
        }

        _oneTimeCommandQueue.waitIdle();

        _context.GetVkDevice().freeCommandBuffers(_context.GetVkGraphicsCommandPool(), 1, &_oneTimeCommandBuffer);
        _oneTimeCommandBuffer = nullptr;
        _oneTimeCommandQueue = nullptr;
    }

    void VkCommandQueue::CopyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset) {
        if (!_oneTimeCommandBuffer) {
            Log::Error("No one-time command buffer is currently active.");
            return;
        }

        vk::BufferCopy copyRegion;
        copyRegion.size = size;
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;

        _oneTimeCommandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    }

    void VkCommandQueue::CopyBuffer(const vk::Buffer& srcBuffer, const vk::Image& dstImage, uint32_t width, uint32_t height, uint32_t srcOffset, uint32_t dstOffset, uint32_t arrayLayer) {
        if (!_oneTimeCommandBuffer) {
            Log::Error("No one-time command buffer is currently active.");
            return;
        }

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

        _oneTimeCommandBuffer.copyBufferToImage(srcBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, 1, &copyRegion);
    }

    void VkCommandQueue::CopyBuffer(const Ref<VertexBuffer>& srcBuffer, const Ref<VertexBuffer>& dstBuffer, uint32_t size, uint32_t srcOffset, uint32_t dstOffset) {
        auto vkSrcBuffer = std::dynamic_pointer_cast<VkVertexBuffer>(srcBuffer);
        auto vkDstBuffer = std::dynamic_pointer_cast<VkVertexBuffer>(dstBuffer);
        FASSERT(vkSrcBuffer && vkDstBuffer, "Invalid vertex buffer type for Vulkan command queue");

        CopyBuffer(vkSrcBuffer->GetVkBuffer(), vkDstBuffer->GetVkBuffer(), size, srcOffset, dstOffset);
    }

    void VkCommandQueue::TransitionImageLayout( const vk::Image& image, 
                                                vk::ImageAspectFlags aspectMask, 
                                                vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                                vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
                                                vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask )
    {
        if (!_oneTimeCommandBuffer) {
            Log::Error("No one-time command buffer is currently active.");
            return;
        }
        
        vk::ImageMemoryBarrier barrier;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;

        _oneTimeCommandBuffer.pipelineBarrier(srcStageMask, dstStageMask, vk::DependencyFlags(), nullptr, nullptr, barrier);
    }

	void VkCommandQueue::GenerateMipmaps(const vk::Image& image,
		                                vk::ImageAspectFlags aspectMask,
		                                vk::Format format, uint32_t width, uint32_t height,
		                                uint32_t arrayLayer,
		                                uint32_t mipLevels,
		                                vk::ImageLayout& oldLayout,
		                                vk::AccessFlags& srcAccessMask,
		                                vk::PipelineStageFlags& srcStageMask)
    {
        vk::FormatProperties formatProperties;
        _context.GetVkPhysicalDevice().getFormatProperties(format, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            std::runtime_error("Texture format does not support linear blitting for mipmap generation.");
        }

        if (!_oneTimeCommandBuffer) {
            Log::Error("No one-time command buffer is currently active.");
            return;
        }

        vk::ImageMemoryBarrier barrier;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = arrayLayer;
		barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        // 초기 상태: 밉 레벨 0을 TRANSFER_DST_OPTIMAL에서 TRANSFER_SRC_OPTIMAL로 전환
        // 이 상태는 밉 레벨 0이 첫 번째 블리팅의 소스가 되기 위함입니다.
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
		barrier.oldLayout = oldLayout;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        _oneTimeCommandBuffer.pipelineBarrier(srcStageMask, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, barrier);

        int32_t mipWidth = static_cast<int32_t>(width);
        int32_t mipHeight = static_cast<int32_t>(height);

        for (uint32_t i = 1; i < mipLevels; ++i) {
            vk::ImageBlit blitRegion;
            blitRegion.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
            blitRegion.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
            blitRegion.srcSubresource.aspectMask = aspectMask;
            blitRegion.srcSubresource.mipLevel = i - 1;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = arrayLayer;

            // 다음 밉 레벨의 크기를 계산합니다.
            mipWidth = (mipWidth > 1) ? mipWidth / 2 : 1;
            mipHeight = (mipHeight > 1) ? mipHeight / 2 : 1;

            blitRegion.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
            blitRegion.dstOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
            blitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blitRegion.dstSubresource.mipLevel = i;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = arrayLayer;

            _oneTimeCommandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, oldLayout, 1, &blitRegion, vk::Filter::eLinear);

            // 현재 밉 레벨(i)을 다음 반복의 소스로 사용하기 위해 TRANSFER_SRC_OPTIMAL로 전환합니다.
            barrier.subresourceRange.baseMipLevel = i;
            barrier.subresourceRange.levelCount = 1;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            _oneTimeCommandBuffer.pipelineBarrier(srcStageMask, vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, barrier);
        }

		oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		srcAccessMask = vk::AccessFlagBits::eTransferRead;
        srcStageMask = vk::PipelineStageFlagBits::eTransfer;
    }
}

#endif