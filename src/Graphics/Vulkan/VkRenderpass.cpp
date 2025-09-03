#include "pch.h"
#include "VkRenderPass.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"

namespace flaw {
    VkRenderPassLayout::VkRenderPassLayout(VkContext &context, const Descriptor &descriptor)
        : _context(context)
		, _pipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        , _sampleCount(descriptor.sampleCount)
    {
        for (uint32_t i = 0; i < descriptor.colorAttachments.size(); ++i) {
            const auto &attachment = descriptor.colorAttachments[i];

            vk::AttachmentReference resolveAttachmentRef;
			resolveAttachmentRef.attachment = static_cast<uint32_t>(_vkAttachments.size());
			resolveAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
            
            vk::AttachmentDescription attachmentDescription;
            attachmentDescription.format = ConvertToVkFormat(attachment.format);
            attachmentDescription.samples = ConvertToVkSampleCount(descriptor.sampleCount);
            attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
            attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
            attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
            attachmentDescription.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

            _vkAttachments.push_back(attachmentDescription);
            _colorAttachmentRefs.push_back(resolveAttachmentRef);
        }

        if (descriptor.depthStencilAttachment.has_value()) {
            const auto &depthAttachment = descriptor.depthStencilAttachment.value();

            vk::AttachmentReference depthAttachmentRef;
            depthAttachmentRef.attachment = static_cast<uint32_t>(_vkAttachments.size());
            depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            vk::AttachmentDescription depthAttachmentDescription;
            depthAttachmentDescription.format = ConvertToVkFormat(depthAttachment.format);
            depthAttachmentDescription.samples = ConvertToVkSampleCount(descriptor.sampleCount);
            depthAttachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
            depthAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
            depthAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            depthAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            depthAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
            depthAttachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            _vkAttachments.push_back(depthAttachmentDescription);
            _depthAttachmentRef = depthAttachmentRef;
        }

        for (uint32_t i = 0; i < descriptor.resolveAttachments.size(); ++i) {
            const auto& resolveAttachment = descriptor.resolveAttachments[i];

            vk::AttachmentReference resolveAttachmentRef;
            resolveAttachmentRef.attachment = static_cast<uint32_t>(_vkAttachments.size());
            resolveAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            vk::AttachmentDescription resolveAttachmentDescription;
            resolveAttachmentDescription.format = ConvertToVkFormat(resolveAttachment.format);
            resolveAttachmentDescription.samples = vk::SampleCountFlagBits::e1;
            resolveAttachmentDescription.loadOp = vk::AttachmentLoadOp::eDontCare;
            resolveAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
            resolveAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            resolveAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            resolveAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
            resolveAttachmentDescription.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

            _vkAttachments.push_back(resolveAttachmentDescription);
			_resolveAttachmentRefs.push_back(resolveAttachmentRef);
        }
    }

    uint32_t VkRenderPassLayout::GetColorAttachmentCount() const {
        return _colorAttachmentRefs.size();
    }

	VkRenderPassLayout::Attachment VkRenderPassLayout::GetColorAttachment(uint32_t index) const {
		if (index >= _colorAttachmentRefs.size()) {
			throw std::runtime_error("Color attachment index out of bounds.");
		}

		uint32_t attachmentIndex = _colorAttachmentRefs[index].attachment;

        return { ConvertToPixelFormat(_vkAttachments[attachmentIndex].format) };
	}

    bool VkRenderPassLayout::HasDepthStencilAttachment() const {
        return _depthAttachmentRef.has_value();
    }

    VkRenderPassLayout::Attachment VkRenderPassLayout::GetDepthStencilAttachment() const {
        if (!HasDepthStencilAttachment()) {
			throw std::runtime_error("No depth-stencil attachment in this render pass layout.");
        }

		return { ConvertToPixelFormat(_vkAttachments[_depthAttachmentRef->attachment].format) };
    }

    uint32_t VkRenderPassLayout::GetResolveAttachmentCount() const {
        return _resolveAttachmentRefs.size();
    }

	VkRenderPassLayout::Attachment VkRenderPassLayout::GetResolveAttachment(uint32_t index) const {
		if (index >= _resolveAttachmentRefs.size()) {
			throw std::runtime_error("Resolve attachment index out of bounds.");
		}

		uint32_t attachmentIndex = _resolveAttachmentRefs[index].attachment;

		return { ConvertToPixelFormat(_vkAttachments[attachmentIndex].format) };
    }

    uint32_t VkRenderPassLayout::GetSampleCount() const {
        return _sampleCount;
    }

    VkRenderPass::VkRenderPass(VkContext &context, const Descriptor &descriptor)
        : _context(context)
		, _layout(std::static_pointer_cast<VkRenderPassLayout>(descriptor.layout))
    {
		FASSERT(_layout, "VkRenderPassLayout is not set in the descriptor.");

        if (!CreateRenderPass(descriptor)) {
            Log::Fatal("Failed to create Vulkan render pass.");
            return;
        }

        _colorAttachmentOps = descriptor.colorAttachmentOps;
        _depthStencilOp = descriptor.depthStencilAttachmentOp;
		_resolveAttachmentOps = descriptor.resolveAttachmentOps;
    }

    VkRenderPass::~VkRenderPass() {
        _context.AddDelayedDeletionTasks([&context = _context, renderPass = _renderPass]() { 
            context.GetVkDevice().destroyRenderPass(renderPass, nullptr); 
        });
    }

    const RenderPass::ColorAttachmentOperation& VkRenderPass::GetColorAttachmentOp(uint32_t index) const {
        return _colorAttachmentOps.at(index);
    }

    const RenderPass::DepthStencilAttachmentOperation& VkRenderPass::GetDepthStencilAttachmentOp() const {
        return _depthStencilOp.value();
    }

    const RenderPass::ResolveAttachmentOperation& VkRenderPass::GetResolveAttachmentOp(uint32_t index) const {
        return _resolveAttachmentOps.at(index);
    }

    bool VkRenderPass::CreateRenderPass(const Descriptor &descriptor) {
        vk::SubpassDescription subpassDescription;
        subpassDescription.pipelineBindPoint = _layout->GetVkPipelineBindPoint();

        std::vector<vk::AttachmentDescription> attachments = _layout->GetVkAttachments();

        std::vector<vk::AttachmentReference> colorAttachmentRefs = _layout->GetVkColorAttachmentRefs();
        for (uint32_t i = 0; i < colorAttachmentRefs.size(); ++i) {
			auto& colorAttachmentRef = colorAttachmentRefs[i];
			auto& attachment = attachments[colorAttachmentRef.attachment];
            auto& operation = descriptor.colorAttachmentOps[i];

            attachment.loadOp = ConvertToVkAttachmentLoadOp(operation.loadOp);
            attachment.storeOp = ConvertToVkAttachmentStoreOp(operation.storeOp);
            attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            attachment.initialLayout = ConvertToVkImageLayout(operation.initialLayout);
            attachment.finalLayout = ConvertToVkImageLayout(operation.finalLayout);
        }

        subpassDescription.colorAttachmentCount = colorAttachmentRefs.size();
        subpassDescription.pColorAttachments = colorAttachmentRefs.data();

        vk::AttachmentReference depthStencilAttachmentRef;
        if (_layout->HasDepthStencilAttachment() && descriptor.depthStencilAttachmentOp.has_value()) {
            depthStencilAttachmentRef = _layout->GetVkDepthAttachmentRef();
			auto& attachment = attachments[depthStencilAttachmentRef.attachment];
            auto& operation = descriptor.depthStencilAttachmentOp.value();

            attachment.loadOp = ConvertToVkAttachmentLoadOp(operation.loadOp);
            attachment.storeOp = ConvertToVkAttachmentStoreOp(operation.storeOp);
            attachment.stencilLoadOp = ConvertToVkAttachmentLoadOp(operation.stencilLoadOp);
            attachment.stencilStoreOp = ConvertToVkAttachmentStoreOp(operation.stencilStoreOp);
            attachment.initialLayout = ConvertToVkImageLayout(operation.initialLayout);
            attachment.finalLayout = ConvertToVkImageLayout(operation.finalLayout);

            subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentRef;
        }

        std::vector<vk::AttachmentReference> resolveAttachmentRefs = _layout->GetVkResolveAttachmentRefs();
        for (uint32_t i = 0; i < resolveAttachmentRefs.size(); ++i) {
			auto& resolveAttachmentRef = resolveAttachmentRefs[i];
			auto& attachment = attachments[resolveAttachmentRef.attachment];
            auto &operation = descriptor.resolveAttachmentOps[i];

            attachment.loadOp = ConvertToVkAttachmentLoadOp(operation.loadOp);
            attachment.storeOp = ConvertToVkAttachmentStoreOp(operation.storeOp);
            attachment.initialLayout = ConvertToVkImageLayout(operation.initialLayout);
            attachment.finalLayout = ConvertToVkImageLayout(operation.finalLayout);
        }

		subpassDescription.pResolveAttachments = resolveAttachmentRefs.data();

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;

        auto renderPassWrapper = _context.GetVkDevice().createRenderPass(renderPassInfo, nullptr);
        if (renderPassWrapper.result != vk::Result::eSuccess) {
            LOG_FATAL("Failed to create Vulkan render pass: %s", vk::to_string(renderPassWrapper.result).c_str());
            return false;
        }

        _renderPass = renderPassWrapper.value;

        return true;
    }
}

#endif