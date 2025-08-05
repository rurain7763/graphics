#include "pch.h"
#include "VkRenderPass.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"

namespace flaw
{
    VkRenderPassLayout::VkRenderPassLayout(VkContext &context, const Descriptor &descriptor)
        : _context(context)
        , _pipelineBindPoint(ConvertToVkPipelineBindPoint(descriptor.type))
        , _colorAttachments(descriptor.colorAttachments)
        , _depthStencilAttachment(descriptor.depthStencilAttachment)
        , _resolveAttachment(descriptor.resolveAttachment)
    {
        for (uint32_t i = 0; i < descriptor.colorAttachments.size(); ++i)
        {
            const auto &attachment = descriptor.colorAttachments[i];

            vk::AttachmentDescription attachmentDescription;
            attachmentDescription.format = ConvertToVkFormat(attachment.format);
            attachmentDescription.samples = ConvertToVkSampleCount(attachment.sampleCount);
            attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
            attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
            attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
            attachmentDescription.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

            _vkAttachments.push_back(attachmentDescription);
            _colorAttachmentRefs.push_back({static_cast<uint32_t>(i), vk::ImageLayout::eColorAttachmentOptimal});
        }

        if (descriptor.depthStencilAttachment.has_value())
        {
            const auto &depthAttachment = descriptor.depthStencilAttachment.value();

            vk::AttachmentReference depthAttachmentRef;
            depthAttachmentRef.attachment = static_cast<uint32_t>(_vkAttachments.size());
            depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            vk::AttachmentDescription depthAttachmentDescription;
            depthAttachmentDescription.format = ConvertToVkFormat(depthAttachment.format);
            depthAttachmentDescription.samples = ConvertToVkSampleCount(depthAttachment.sampleCount);
            depthAttachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
            depthAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
            depthAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            depthAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            depthAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
            depthAttachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            _vkAttachments.push_back(depthAttachmentDescription);
            _depthAttachmentRef = depthAttachmentRef;
        }

        if (descriptor.resolveAttachment.has_value())
        {
            const auto &resolveAttachment = descriptor.resolveAttachment.value();

            vk::AttachmentReference resolveAttachmentRef;
            resolveAttachmentRef.attachment = static_cast<uint32_t>(_vkAttachments.size());
            resolveAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

            vk::AttachmentDescription resolveAttachmentDescription;
            resolveAttachmentDescription.format = ConvertToVkFormat(resolveAttachment.format);
            resolveAttachmentDescription.samples = ConvertToVkSampleCount(resolveAttachment.sampleCount);
            resolveAttachmentDescription.loadOp = vk::AttachmentLoadOp::eDontCare;
            resolveAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
            resolveAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            resolveAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            resolveAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
            resolveAttachmentDescription.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

            _vkAttachments.push_back(resolveAttachmentDescription);
            _resolveAttachmentRef = resolveAttachmentRef;
        }
    }

    uint32_t VkRenderPassLayout::GetColorAttachmentCount() const {
        return static_cast<uint32_t>(_colorAttachmentRefs.size());
    }

    const VkRenderPassLayout::ColorAttachment &VkRenderPassLayout::GetColorAttachment(uint32_t index) const {
        return _colorAttachments.at(index);
    }

    bool VkRenderPassLayout::HasDepthStencilAttachment() const {
        return _depthAttachmentRef.has_value();
    }

    const VkRenderPassLayout::DepthStencilAttachment &VkRenderPassLayout::GetDepthStencilAttachment() const {
        return _depthStencilAttachment.value();
    }

    bool VkRenderPassLayout::HasResolveAttachment() const {
        return _resolveAttachmentRef.has_value();
    }

    const VkRenderPassLayout::ResolveAttachment &VkRenderPassLayout::GetResolveAttachment() const {
        return _resolveAttachment.value();
    }

    VkRenderPass::VkRenderPass(VkContext &context, const Descriptor &descriptor)
        : _context(context)
    {
        if (!CreateRenderPass(descriptor))
        {
            Log::Fatal("Failed to create Vulkan render pass.");
            return;
        }

        _colorOperations = descriptor.colorAttachmentOperations;
        _depthStencilOperation = descriptor.depthStencilAttachmentOperation;
    }

    VkRenderPass::~VkRenderPass()
    {
        _context.AddDelayedDeletionTasks([&context = _context, renderPass = _renderPass]()
                                         { context.GetVkDevice().destroyRenderPass(renderPass, nullptr); });
    }

    uint32_t VkRenderPass::GetColorAttachmentOpCount() const {
        return static_cast<uint32_t>(_colorOperations.size());
    }

    const GraphicsRenderPass::ColorAttachmentOperation& VkRenderPass::GetColorAttachmentOp(uint32_t index) const {
        return _colorOperations.at(index);
    }

    bool VkRenderPass::HasDepthStencilAttachmentOp() const {
        return _depthStencilOperation.has_value();
    }

    const GraphicsRenderPass::DepthStencilAttachmentOperation& VkRenderPass::GetDepthStencilAttachmentOp() const {
        return _depthStencilOperation.value();
    }

    bool VkRenderPass::HasResolveAttachmentOp() const {
        return _resolveAttachmentOperation.has_value();
    }

    const GraphicsRenderPass::ResolveAttachmentOperation& VkRenderPass::GetResolveAttachmentOp() const {
        return _resolveAttachmentOperation.value();
    }

    bool VkRenderPass::CreateRenderPass(const Descriptor &descriptor) {
        auto vkRenderPassLayout = std::static_pointer_cast<VkRenderPassLayout>(descriptor.layout);
        if (!vkRenderPassLayout)
        {
            Log::Fatal("VkRenderPassLayout is not set in the descriptor.");
            return false;
        }

        vk::SubpassDescription subpassDescription;
        subpassDescription.pipelineBindPoint = vkRenderPassLayout->GetVkPipelineBindPoint();

        std::vector<vk::AttachmentDescription> attachments = vkRenderPassLayout->GetVkAttachments();

        std::vector<vk::AttachmentReference> colorAttachmentRefs = vkRenderPassLayout->GetVkColorAttachmentRefs();
        for (uint32_t i = 0; i < colorAttachmentRefs.size(); ++i)
        {
            auto &attachment = attachments[i];
            auto &operation = descriptor.colorAttachmentOperations[i];

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
        if (vkRenderPassLayout->HasDepthStencilAttachment() && descriptor.depthStencilAttachmentOperation.has_value())
        {
            depthStencilAttachmentRef = vkRenderPassLayout->GetVkDepthAttachmentRef();

            auto &attachment = *(attachments.begin() + colorAttachmentRefs.size());
            auto &operation = descriptor.depthStencilAttachmentOperation.value();

            attachment.loadOp = ConvertToVkAttachmentLoadOp(operation.loadOp);
            attachment.storeOp = ConvertToVkAttachmentStoreOp(operation.storeOp);
            attachment.stencilLoadOp = ConvertToVkAttachmentLoadOp(operation.stencilLoadOp);
            attachment.stencilStoreOp = ConvertToVkAttachmentStoreOp(operation.stencilStoreOp);
            attachment.initialLayout = ConvertToVkImageLayout(operation.initialLayout);
            attachment.finalLayout = ConvertToVkImageLayout(operation.finalLayout);

            subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentRef;
        }

        vk::AttachmentReference resolveAttachmentRef;
        if (vkRenderPassLayout->HasResolveAttachment() && descriptor.resolveAttachmentOperation.has_value())
        {
            resolveAttachmentRef = vkRenderPassLayout->GetVkResolveAttachmentRef();

            auto &attachment = *(attachments.begin() + colorAttachmentRefs.size() + (vkRenderPassLayout->HasDepthStencilAttachment() ? 1 : 0));
            auto &operation = descriptor.resolveAttachmentOperation.value();

            attachment.loadOp = ConvertToVkAttachmentLoadOp(operation.loadOp);
            attachment.storeOp = ConvertToVkAttachmentStoreOp(operation.storeOp);
            attachment.initialLayout = ConvertToVkImageLayout(operation.initialLayout);
            attachment.finalLayout = ConvertToVkImageLayout(operation.finalLayout);

            subpassDescription.pResolveAttachments = &resolveAttachmentRef;
        }

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;

        auto renderPassWrapper = _context.GetVkDevice().createRenderPass(renderPassInfo, nullptr);
        if (renderPassWrapper.result != vk::Result::eSuccess)
        {
            Log::Fatal("Failed to create Vulkan render pass: %s", vk::to_string(renderPassWrapper.result).c_str());
            return false;
        }

        _renderPass = renderPassWrapper.value;

        return true;
    }
}

#endif