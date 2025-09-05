#include "pch.h"
#include "VkRenderPass.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"

namespace flaw {
    VkRenderPass::VkRenderPass(VkContext& context, const Descriptor& descriptor)
	    : _context(context)
		, _renderPass(VK_NULL_HANDLE)
		, _attachments(descriptor.attachments)
		, _subpasses(descriptor.subpasses)
		, _dependencies(descriptor.dependencies)
    {
		std::vector<vk::AttachmentDescription> attachmentDescriptions(descriptor.attachments.size());
		std::vector<vk::SubpassDescription> subPassDescriptions(descriptor.subpasses.size());
		std::vector<std::vector<vk::AttachmentReference>> inputAttachmentRefsList(descriptor.subpasses.size());
		std::vector<std::vector<vk::AttachmentReference>> colorAttachmentRefsList(descriptor.subpasses.size());
		std::vector<vk::AttachmentReference> depthAttachmentRefsList(descriptor.subpasses.size());
		std::vector<std::vector<vk::AttachmentReference>> resolveAttachmentRefsList(descriptor.subpasses.size());
		std::vector<vk::SubpassDependency> dependencies(descriptor.dependencies.size());

		_clearValues.resize(_attachments.size());
        for (uint32_t i = 0; i < _attachments.size(); i++) {
			const auto& attachment = _attachments[i];

			vk::AttachmentDescription& attachmentDescription = attachmentDescriptions[i];
			attachmentDescription.format = ConvertToVkFormat(attachment.format);
            attachmentDescription.samples = ConvertToVkSampleCount(attachment.sampleCount);
			attachmentDescription.loadOp = ConvertToVkAttachmentLoadOp(attachment.loadOp);
			attachmentDescription.storeOp = ConvertToVkAttachmentStoreOp(attachment.storeOp);
			attachmentDescription.stencilLoadOp = ConvertToVkAttachmentLoadOp(attachment.stencilLoadOp);
			attachmentDescription.stencilStoreOp = ConvertToVkAttachmentStoreOp(attachment.stencilStoreOp);
			attachmentDescription.initialLayout = ConvertToVkImageLayout(attachment.initialLayout);
			attachmentDescription.finalLayout = ConvertToVkImageLayout(attachment.finalLayout);

            switch (attachment.format)
            {
                case PixelFormat::D24S8_UINT:
                case PixelFormat::D32F:
			    case PixelFormat::D32F_S8UI:
				    _clearValues[i].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
				    break;
			    default:
				    _clearValues[i].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
				    break;
            }
		}

        for (uint32_t i = 0; i < _subpasses.size(); i++) {
			const auto& subpass = _subpasses[i];

			vk::SubpassDescription& subPassDescription = subPassDescriptions[i];
			subPassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

			auto& inputAttachmentRefs = inputAttachmentRefsList[i];
			for (const auto& inputRef : subpass.inputAttachmentRefs) {
				vk::AttachmentReference vkInputRef;
				vkInputRef.attachment = inputRef.attachmentIndex;
				vkInputRef.layout = ConvertToVkImageLayout(inputRef.layout);
				inputAttachmentRefs.push_back(vkInputRef);
			}

			subPassDescription.inputAttachmentCount = subpass.inputAttachmentRefs.size();
			subPassDescription.pInputAttachments = inputAttachmentRefs.data();

			auto& colorAttachmentRefs = colorAttachmentRefsList[i];
            for (const auto& colorRef : subpass.colorAttachmentRefs) {
                vk::AttachmentReference vkColorRef;
                vkColorRef.attachment = colorRef.attachmentIndex;
                vkColorRef.layout = ConvertToVkImageLayout(colorRef.layout);
                colorAttachmentRefs.push_back(vkColorRef);
            }

			subPassDescription.colorAttachmentCount = subpass.colorAttachmentRefs.size();
			subPassDescription.pColorAttachments = colorAttachmentRefs.data();

            auto& resolveAttachmentRefs = resolveAttachmentRefsList[i];
            for (const auto& resolveRef : subpass.resolveAttachmentRefs) {
                vk::AttachmentReference vkResolveRef;
                vkResolveRef.attachment = resolveRef.attachmentIndex;
                vkResolveRef.layout = ConvertToVkImageLayout(resolveRef.layout);
                resolveAttachmentRefs.push_back(vkResolveRef);
            }

            subPassDescription.pResolveAttachments = resolveAttachmentRefs.data();

            if (subpass.depthStencilAttachmentRef.has_value()) {
                auto& depthRef = depthAttachmentRefsList[i];
                depthRef.attachment = subpass.depthStencilAttachmentRef->attachmentIndex;
                depthRef.layout = ConvertToVkImageLayout(subpass.depthStencilAttachmentRef->layout);
                subPassDescription.pDepthStencilAttachment = &depthRef;
            }

			subPassDescription.preserveAttachmentCount = 0;
			subPassDescription.pPreserveAttachments = nullptr;
        }

        for (uint32_t i = 0; i < _dependencies.size(); i++) {
			const auto& dependency = _dependencies[i];

			vk::SubpassDependency& vkDependency = dependencies[i];
			vkDependency.srcSubpass = dependency.srcSubPass;
			vkDependency.dstSubpass = dependency.dstSubPass;
			vkDependency.srcAccessMask = ConvertToVkAccessFlags(dependency.srcAccessTypes);
			vkDependency.dstAccessMask = ConvertToVkAccessFlags(dependency.dstAccessTypes);
			vkDependency.srcStageMask = ConvertToVkPipelineStageFlags(dependency.srcPipeStages);
			vkDependency.dstStageMask = ConvertToVkPipelineStageFlags(dependency.dstPipeStages);
        }

		vk::RenderPassCreateInfo renderPassInfo;
		renderPassInfo.attachmentCount = attachmentDescriptions.size();
		renderPassInfo.pAttachments = attachmentDescriptions.data();
		renderPassInfo.subpassCount = subPassDescriptions.size();
		renderPassInfo.pSubpasses = subPassDescriptions.data();
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();

		auto renderPassWrapper = _context.GetVkDevice().createRenderPass(renderPassInfo, nullptr);
        if (renderPassWrapper.result != vk::Result::eSuccess) {
            LOG_FATAL("Failed to create Vulkan render pass: %s", vk::to_string(renderPassWrapper.result).c_str());
            return;
        }

		_renderPass = renderPassWrapper.value;
    }

    VkRenderPass::~VkRenderPass() {
        _context.AddDelayedDeletionTasks([&context = _context, renderPass = _renderPass]() {
            context.GetVkDevice().destroyRenderPass(renderPass);
        });
    }
}

#endif