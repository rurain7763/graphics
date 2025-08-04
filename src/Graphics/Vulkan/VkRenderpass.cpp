#include "pch.h"
#include "VkRenderPass.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"

namespace flaw {
    VkRenderPassLayout::VkRenderPassLayout(VkContext& context, const Descriptor& descriptor)
        : _context(context)
        , _pipelineBindPoint(ConvertToVkPipelineBindPoint(descriptor.type)) 
        , _colorAttachments(descriptor.colorAttachments)
    {
        for (uint32_t i = 0; i < descriptor.colorAttachments.size(); ++i) {
            const auto& attachment = descriptor.colorAttachments[i];

            vk::AttachmentDescription attachmentDescription;
            attachmentDescription.format = ConvertToVkFormat(attachment.format);
            attachmentDescription.samples = vk::SampleCountFlagBits::e1;
            attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
            attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
            attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
            attachmentDescription.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

            _vkColorAttachments.push_back(attachmentDescription);
            _colorAttachmentRefs.push_back({ static_cast<uint32_t>(i), vk::ImageLayout::eColorAttachmentOptimal });
        }

        if (descriptor.depthStencilAttachment.has_value()) {
            const auto& depthAttachment = descriptor.depthStencilAttachment.value();

            vk::AttachmentReference depthAttachmentRef;
            depthAttachmentRef.attachment = static_cast<uint32_t>(_vkColorAttachments.size());
            depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            vk::AttachmentDescription depthAttachmentDescription;
            depthAttachmentDescription.format = ConvertToVkFormat(depthAttachment.format);
            depthAttachmentDescription.samples = vk::SampleCountFlagBits::e1;
            depthAttachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
            depthAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
            depthAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            depthAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            depthAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
            depthAttachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            _vkColorAttachments.push_back(depthAttachmentDescription);
            _depthAttachmentRef = depthAttachmentRef;
        }
    }

    uint32_t VkRenderPassLayout::GetColorAttachmentCount() const {
        return static_cast<uint32_t>(_colorAttachmentRefs.size());
    }

    const VkRenderPassLayout::ColorAttachment& VkRenderPassLayout::GetColorAttachment(uint32_t index) const {
        return _colorAttachments.at(index); 
    }

    bool VkRenderPassLayout::HasDepthStencilAttachment() const {
        return _depthAttachmentRef.has_value();
    }

    VkRenderPass::VkRenderPass(VkContext& context, const Descriptor& descriptor)
        : _context(context)
    {
        if (!CreateRenderPass(descriptor)) {
            Log::Fatal("Failed to create Vulkan render pass.");
            return;
        }

        _colorOperations = descriptor.colorAttachmentOperations;
        _depthStencilOperation = descriptor.depthStencilAttachmentOperation;
    }

    VkRenderPass::~VkRenderPass() {
        _context.AddDelayedDeletionTasks([&context = _context, renderPass = _renderPass]() {
            context.GetVkDevice().destroyRenderPass(renderPass, nullptr, context.GetVkDispatchLoader());
        });
    }

    uint32_t VkRenderPass::GetColorAttachmentCount() const {
        return _colorOperations.size();
    }

    AttachmentLoadOp VkRenderPass::GetColorAttachmentLoadOp(uint32_t index) const {
        return _colorOperations.at(index).loadOp;
    }

    bool VkRenderPass::HasDepthStencilAttachment() const {
        return _depthStencilOperation.has_value();
    }

    AttachmentLoadOp VkRenderPass::GetDepthStencilAttachmentLoadOp() const {
        if (_depthStencilOperation.has_value()) {
            return _depthStencilOperation->loadOp;
        }

        throw std::runtime_error("No depth-stencil attachment operation defined.");
    }

    bool VkRenderPass::CreateRenderPass(const Descriptor& descriptor) {
        auto vkRenderPassLayout = std::static_pointer_cast<VkRenderPassLayout>(descriptor.layout);
        if (!vkRenderPassLayout) {
            Log::Fatal("VkRenderPassLayout is not set in the descriptor.");
            return false;
        }

        vk::SubpassDescription subpassDescription;
        subpassDescription.pipelineBindPoint = vkRenderPassLayout->GetVkPipelineBindPoint();

        std::vector<vk::AttachmentDescription> attachments = vkRenderPassLayout->GetVkAttachments();

        std::vector<vk::AttachmentReference> colorAttachmentRefs = vkRenderPassLayout->GetVkColorAttachmentRefs();
        for (uint32_t i = 0; i < colorAttachmentRefs.size(); ++i) {
            auto& attachment = attachments[i];
            auto& operation = descriptor.colorAttachmentOperations[i];

            attachment.loadOp = ConvertToVkAttachmentLoadOp(operation.loadOp);
            attachment.storeOp = ConvertToVkAttachmentStoreOp(operation.storeOp);
            attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            attachment.initialLayout = ConvertToVkImageLayout(operation.initialLayout);
            attachment.finalLayout = ConvertToVkImageLayout(operation.finalLayout);
        }

        subpassDescription.colorAttachmentCount = colorAttachmentRefs.size();
        subpassDescription.pColorAttachments = colorAttachmentRefs.data();
        
        auto depthStencilAttachmentRefs = vkRenderPassLayout->GetVkDepthAttachmentRef();
        if (depthStencilAttachmentRefs.has_value() && descriptor.depthStencilAttachmentOperation.has_value()) {
            auto& attachment = attachments.back();
            auto& operation = descriptor.depthStencilAttachmentOperation.value();

            attachment.loadOp = ConvertToVkAttachmentLoadOp(operation.loadOp);
            attachment.storeOp = ConvertToVkAttachmentStoreOp(operation.storeOp);
            attachment.stencilLoadOp = ConvertToVkAttachmentLoadOp(operation.stencilLoadOp);
            attachment.stencilStoreOp = ConvertToVkAttachmentStoreOp(operation.stencilStoreOp);
            attachment.initialLayout = ConvertToVkImageLayout(operation.initialLayout);
            attachment.finalLayout = ConvertToVkImageLayout(operation.finalLayout);

            subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentRefs.value();
        }

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;

        auto renderPassWrapper = _context.GetVkDevice().createRenderPass(renderPassInfo, nullptr, _context.GetVkDispatchLoader());
        if (renderPassWrapper.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan render pass: %s", vk::to_string(renderPassWrapper.result).c_str());
            return false;
        }

        _renderPass = renderPassWrapper.value;

        return true;
    }
}

#endif