#include "pch.h"
#include "VkFramebuffer.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "VkTextures.h"
#include "VkRenderPass.h"
#include "Log/Log.h"

namespace flaw {
    VkFramebuffer::VkFramebuffer(VkContext& context, const Descriptor& descriptor)
        : _context(context)
        , _extent{ descriptor.width, descriptor.height }
        , _colorAttachments(descriptor.colorAttachments)
        , _depthStencilAttachment(descriptor.depthStencilAttachment.has_value() ? descriptor.depthStencilAttachment.value() : nullptr)
        , _renderPassLayout(std::static_pointer_cast<VkRenderPassLayout>(descriptor.renderPassLayout))
        , _colorResizeHandler(descriptor.colorResizeHandler ? descriptor.colorResizeHandler : nullptr)
        , _depthStencilResizeHandler(descriptor.depthStencilResizeHandler ? descriptor.depthStencilResizeHandler : nullptr)
    {
        if (_renderPassLayout == nullptr) {
            Log::Fatal("Render pass layout cannot be null for VkFramebuffer.");
            return;
        }

        if (!CreateRenderPass()) {
            Log::Fatal("Failed to create Vulkan render pass for framebuffer.");
            return;
        }

        if (!CreateFramebuffer()) {
            Log::Fatal("Failed to create Vulkan framebuffer.");
            return;
        }
    }

    VkFramebuffer::~VkFramebuffer() {
        _context.AddDelayedDeletionTasks([&context = _context, framebuffer = _framebuffer]() {
            context.GetVkDevice().destroyFramebuffer(framebuffer, nullptr);
        });
    }

    void VkFramebuffer::Resize(uint32_t width, uint32_t height) {
        if (width == _extent.width && height == _extent.height) {
            return;
        }

        bool needRecreate = false;

        if (_colorResizeHandler) {
            for (uint32_t i = 0; i < _colorAttachments.size(); ++i) {
                auto newAttachment = _colorResizeHandler(i, width, height);
                needRecreate |= newAttachment != _colorAttachments[i];
                _colorAttachments[i] = newAttachment;
            }
        }
        
        if (_depthStencilResizeHandler && _depthStencilAttachment) {
            auto newAttachment = _depthStencilResizeHandler(width, height);
            needRecreate |= newAttachment != _depthStencilAttachment;
            _depthStencilAttachment = newAttachment;
        }

        if (!needRecreate) {
            return;
        }

        _extent.width = width;
        _extent.height = height;

        _context.AddDelayedDeletionTasks([&context = _context, framebuffer = _framebuffer]() {
            context.GetVkDevice().destroyFramebuffer(framebuffer, nullptr);
        });

        if (!CreateFramebuffer()) {
            Log::Fatal("Failed to resize Vulkan framebuffer.");
        }
    }

	Ref<GraphicsRenderPassLayout> VkFramebuffer::GetRenderPassLayout() const {
		return _renderPassLayout;
	}

    bool VkFramebuffer::CreateRenderPass() {
        GraphicsRenderPass::Descriptor renderPassDesc;
        renderPassDesc.layout = _renderPassLayout;
        renderPassDesc.colorAttachmentOperations.resize(_renderPassLayout->GetColorAttachmentCount());
        for (uint32_t i = 0; i < renderPassDesc.colorAttachmentOperations.size(); ++i) {
            auto& op = renderPassDesc.colorAttachmentOperations[i];
            op.initialLayout = TextureLayout::Undefined;
            op.finalLayout = TextureLayout::Color;
            op.loadOp = AttachmentLoadOp::Clear;
            op.storeOp = AttachmentStoreOp::Store;
        }

        renderPassDesc.depthStencilAttachmentOperation = std::nullopt;
        if (_renderPassLayout->HasDepthStencilAttachment()) {
            renderPassDesc.depthStencilAttachmentOperation = {
                TextureLayout::Undefined,
                TextureLayout::DepthStencil,
                AttachmentLoadOp::Clear,
                AttachmentStoreOp::Store,
                AttachmentLoadOp::DontCare,
                AttachmentStoreOp::DontCare
            };
        }

        _renderpass = CreateRef<VkRenderPass>(_context, renderPassDesc);

        return true;
    }

    bool VkFramebuffer::CreateFramebuffer() {
        std::vector<vk::ImageView> attachmentViews(_colorAttachments.size());
        for (size_t i = 0; i < _colorAttachments.size(); ++i) {
            auto& colorAttachment = _colorAttachments[i];
            attachmentViews[i] = *static_cast<vk::ImageView*>(colorAttachment->GetRenderTargetView());
        }

        if (_depthStencilAttachment) {
            attachmentViews.push_back(*static_cast<vk::ImageView*>(_depthStencilAttachment->GetDepthStencilView()));
        }

        vk::FramebufferCreateInfo createInfo;
        createInfo.renderPass = _renderpass->GetNativeVkRenderPass();
        createInfo.attachmentCount = attachmentViews.size();
        createInfo.pAttachments = attachmentViews.data();
        createInfo.width = _extent.width;
        createInfo.height = _extent.height;
        createInfo.layers = 1;

        auto result = _context.GetVkDevice().createFramebuffer(createInfo, nullptr);
        if (result.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan framebuffer: %s", vk::to_string(result.result).c_str());
            return false;
        }

        _framebuffer = result.value;

        return true;
    }
}

#endif