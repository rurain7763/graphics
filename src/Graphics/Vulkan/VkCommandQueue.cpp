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

    void VkCommandQueue::SetPipelineBarrier(Ref<Texture> texture, TextureLayout oldLayout, TextureLayout newLayout, AccessTypes srcAccess, AccessTypes dstAccess, PipelineStages srcStage, PipelineStages dstStage) {
		const auto& vkNativeTex = static_cast<const VkNativeTexture&>(texture->GetNativeTexture());
	
		auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

		vk::ImageMemoryBarrier barrier;
		barrier.image = vkNativeTex.image;
		barrier.oldLayout = ConvertToVkImageLayout(oldLayout);
		barrier.newLayout = ConvertToVkImageLayout(newLayout);
		barrier.srcAccessMask = ConvertToVkAccessFlags(srcAccess);
		barrier.dstAccessMask = ConvertToVkAccessFlags(dstAccess);
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = GetVkImageAspectFlags(texture->GetPixelFormat());
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		vk::PipelineStageFlags srcStageFlags = ConvertToVkPipelineStageFlags(srcStage);
		vk::PipelineStageFlags dstStageFlags = ConvertToVkPipelineStageFlags(dstStage);

		commandBuffer.pipelineBarrier(srcStageFlags, dstStageFlags, vk::DependencyFlags(), nullptr, nullptr, barrier);
    }

    void VkCommandQueue::SetPipeline(const Ref<GraphicsPipeline>& pipeline) {
        auto vkPipeline = std::static_pointer_cast<VkGraphicsPipeline>(pipeline);
        FASSERT(vkPipeline, "Invalid pipeline type for Vulkan command queue");
        
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];
		
        _currentPipeline = vkPipeline;
        
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _currentPipeline->GetNativeVkGraphicsPipeline());

        const uint32_t behaviorStates = vkPipeline->GetBehaviorStates();

        if (behaviorStates & GraphicsPipeline::Behavior::AutoResizeViewport) {
			if (_currentBeginInfoStack.empty()) {
				LOG_ERROR("No framebuffer bound for the command queue.");
				return;
			}

			const auto& beginInfo = _currentBeginInfoStack.back();

			vk::Viewport newViewport;
			newViewport.x = 0.0f;
			newViewport.y = 0.0f;
			newViewport.width = static_cast<float>(beginInfo.framebuffer->GetWidth());
			newViewport.height = static_cast<float>(beginInfo.framebuffer->GetHeight());
			newViewport.minDepth = 0.0f;
			newViewport.maxDepth = 1.0f;

            commandBuffer.setViewport(0, 1, &newViewport);
        }

        if (behaviorStates & GraphicsPipeline::Behavior::AutoResizeScissor) {
            if (_currentBeginInfoStack.empty()) {
                LOG_ERROR("No framebuffer bound for the command queue.");
                return;
            }

			const auto& beginInfo = _currentBeginInfoStack.back();

			vk::Rect2D newScissor;
			newScissor.offset = vk::Offset2D{ 0, 0 };
			newScissor.extent = vk::Extent2D{ beginInfo.framebuffer->GetWidth(), beginInfo.framebuffer->GetHeight() };

			commandBuffer.setScissor(0, 1, &newScissor);
        }
    }

    void VkCommandQueue::SetPushConstants(uint32_t rangeIndex, const void* data) {
        if (!_currentPipeline) {
            LOG_ERROR("No pipeline set for the command queue.");
            return;
        }

        const auto& pushConstantRanges = _currentPipeline->GetVkPushConstantRanges();
		if (rangeIndex >= pushConstantRanges.size()) {
			LOG_ERROR("Push constant range index out of bounds.");
			return;
		}

		const auto& pushConstantRange = pushConstantRanges[rangeIndex];
		auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

		commandBuffer.pushConstants(_currentPipeline->GetVkPipelineLayout(), pushConstantRange.stageFlags, pushConstantRange.offset, pushConstantRange.size, data);
    }

    void VkCommandQueue::SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vertexBuffers) {
		auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

		std::vector<vk::Buffer> vkVertexBuffers(vertexBuffers.size());
		std::vector<vk::DeviceSize> vkVertexBufferOffsets(vertexBuffers.size());
        for (uint32_t i = 0; i < vertexBuffers.size(); i++) {
            auto vkVertexBuffer = std::static_pointer_cast<VkVertexBuffer>(vertexBuffers[i]);
            FASSERT(vkVertexBuffer, "Invalid vertex buffer type for Vulkan command queue");

			const auto& vkNativeBuff = static_cast<const VkNativeBuffer&>(vkVertexBuffer->GetNativeBuffer());

            vkVertexBuffers[i] = vkNativeBuff.buffer;
            vkVertexBufferOffsets[i] = 0; // Assuming no offset for simplicity
        }

        commandBuffer.bindVertexBuffers(0, vkVertexBuffers.size(), vkVertexBuffers.data(), vkVertexBufferOffsets.data());
    }

    void VkCommandQueue::ResetVertexBuffers() {
		// NOTE: Nothing to do because vkBindVertexBuffers not allows 0 count buffers
    }

    void VkCommandQueue::SetShaderResources(const std::vector<Ref<ShaderResources>>& shaderResources) {
		if (_currentPipeline == nullptr) {
			LOG_ERROR("No pipeline set for the command queue.");
			return;
		}

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

		std::vector<vk::DescriptorSet> vkDescriptorSets(shaderResources.size());
		for (uint32_t i = 0; i < shaderResources.size(); ++i) {
			auto vkShaderResources = std::static_pointer_cast<VkShaderResources>(shaderResources[i]);
			FASSERT(vkShaderResources, "Invalid shader resources type for Vulkan command queue");

			vkDescriptorSets[i] = vkShaderResources->GetVkDescriptorSet();
		}

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _currentPipeline->GetVkPipelineLayout(), 0, vkDescriptorSets.size(), vkDescriptorSets.data(), 0, nullptr);
    }

    void VkCommandQueue::ResetShaderResources() {
		// NOTE: Nothing to do because vkBindDescriptorSets not allows 0 count sets
    }

    void VkCommandQueue::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
        DrawInstanced(vertexCount, 1, vertexOffset);
    }

    void VkCommandQueue::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t instanceOffset) {
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        commandBuffer.draw(vertexCount, instanceCount, vertexOffset, instanceOffset);
    }

    void VkCommandQueue::DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
        DrawIndexedInstanced(indexBuffer, indexCount, 1, indexOffset, vertexOffset);
    }

    void VkCommandQueue::DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instanceOffset) {
        auto vkIndexBuffer = std::dynamic_pointer_cast<VkIndexBuffer>(indexBuffer);
        FASSERT(vkIndexBuffer, "Invalid index buffer type for Vulkan command queue");

		const auto& vkNativeBuff = static_cast<const VkNativeBuffer&>(vkIndexBuffer->GetNativeBuffer());

        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

        commandBuffer.bindIndexBuffer(vkNativeBuff.buffer, 0, vk::IndexType::eUint32);
        commandBuffer.drawIndexed(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
    }

    void VkCommandQueue::BeginRenderPassImpl(const Ref<VkRenderPass>& renderPass, const Ref<VkFramebuffer>& framebuffer) {
        auto& commandBuffer = _graphicsFrameCommandBuffers[_currentCommandBufferIndex];

		auto layout = renderPass->GetLayout();

        std::vector<vk::ClearValue> clearValues;
        for(uint32_t i = 0; i < layout->GetColorAttachmentCount(); ++i) {
            clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
        }

        if (layout->HasDepthStencilAttachment()) {
			clearValues.push_back(vk::ClearDepthStencilValue{ 1.0f, 0 });
        }

        for (uint32_t i = 0; i < layout->GetResolveAttachmentCount(); ++i) {
            clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
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

    void VkCommandQueue::BeginRenderPass(const Ref<RenderPass>& beginRenderPass, const Ref<RenderPass>& resumeRenderPass, const Ref<Framebuffer>& framebuffer) {
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

    vk::CommandBuffer VkCommandQueue::BeginOneTimeCommands() {
        auto device = _context.GetVkDevice();
        auto graphicsCommandPool = _context.GetVkGraphicsCommandPool();

        vk::CommandBufferAllocateInfo buffAllocInfo;
        buffAllocInfo.commandPool = graphicsCommandPool;
        buffAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        buffAllocInfo.commandBufferCount = 1;

        auto buffWrapper = device.allocateCommandBuffers(buffAllocInfo);
        if (buffWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to allocate one-time command buffer: %s", vk::to_string(buffWrapper.result).c_str());
            return nullptr;
        }

        vk::CommandBuffer commandBuffer = buffWrapper.value[0];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        auto result = commandBuffer.begin(beginInfo);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to begin one-time command buffer: %s", vk::to_string(result).c_str());
            return nullptr;
        }

        return commandBuffer;
    }

    void VkCommandQueue::EndOneTimeCommands(vk::CommandBuffer commandBuffer) {
        auto graphicsQueue = _context.GetVkGraphicsQueue();

        commandBuffer.end();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        auto result = graphicsQueue.submit(1, &submitInfo, nullptr);
        if (result != vk::Result::eSuccess) {
            Log::Fatal("Failed to submit one-time command buffer: %s", vk::to_string(result).c_str());
            return;
        }

        graphicsQueue.waitIdle();

        _context.GetVkDevice().freeCommandBuffers(_context.GetVkGraphicsCommandPool(), 1, &commandBuffer);
    }
}

#endif