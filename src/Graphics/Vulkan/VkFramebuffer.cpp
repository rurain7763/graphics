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
        , _resolveAttachment(descriptor.resolveAttachment.has_value() ? descriptor.resolveAttachment.value() : nullptr)
        , _renderPassLayout(std::static_pointer_cast<VkRenderPassLayout>(descriptor.renderPassLayout))
        , _colorResizeHandler(descriptor.colorResizeHandler ? descriptor.colorResizeHandler : nullptr)
        , _depthStencilResizeHandler(descriptor.depthStencilResizeHandler ? descriptor.depthStencilResizeHandler : nullptr)
        , _resolveResizeHandler(descriptor.resolveResizeHandler ? descriptor.resolveResizeHandler : nullptr)
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

		_context.AddOnResizeHandler(PID(this), std::bind(&VkFramebuffer::Resize, this, std::placeholders::_1, std::placeholders::_2));
    }

    VkFramebuffer::~VkFramebuffer() {
		_context.RemoveOnResizeHandler(PID(this));

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
				needRecreate |= _colorResizeHandler(_colorAttachments[i], width, height);
            }
        }
        
        if (_depthStencilResizeHandler && _depthStencilAttachment) {
			needRecreate |= _depthStencilResizeHandler(_depthStencilAttachment, width, height);
        }

        if (_resolveResizeHandler && _resolveAttachment) {
			needRecreate |= _resolveResizeHandler(_resolveAttachment, width, height);
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

	Ref<RenderPassLayout> VkFramebuffer::GetRenderPassLayout() const {
		return _renderPassLayout;
	}

    bool VkFramebuffer::CreateRenderPass() {
        RenderPass::Descriptor renderPassDesc;
        renderPassDesc.layout = _renderPassLayout;

        renderPassDesc.colorAttachmentOps.resize(_renderPassLayout->GetColorAttachmentCount());
        for (uint32_t i = 0; i < renderPassDesc.colorAttachmentOps.size(); ++i) {
            auto& op = renderPassDesc.colorAttachmentOps[i];
            op.initialLayout = TextureLayout::Undefined;
            op.finalLayout = TextureLayout::ColorAttachment;
            op.loadOp = AttachmentLoadOp::Clear;
            op.storeOp = AttachmentStoreOp::Store;
        }

        if (_renderPassLayout->HasDepthStencilAttachment()) {
            renderPassDesc.depthStencilAttachmentOp = {
                TextureLayout::Undefined,
                TextureLayout::DepthStencilAttachment,
                AttachmentLoadOp::Clear,
                AttachmentStoreOp::Store,
                AttachmentLoadOp::DontCare,
                AttachmentStoreOp::DontCare
            };
        }

        if (_renderPassLayout->HasResolveAttachment()) {
            renderPassDesc.resolveAttachmentOp = {
                TextureLayout::Undefined,
                TextureLayout::ColorAttachment,
                AttachmentLoadOp::Clear,
                AttachmentStoreOp::Store
            };
        }

        _renderpass = CreateRef<VkRenderPass>(_context, renderPassDesc);

        return true;
    }

    bool VkFramebuffer::CreateFramebuffer() {
        std::vector<vk::ImageView> attachmentViews(_colorAttachments.size());
        for (size_t i = 0; i < _colorAttachments.size(); ++i) {
            auto& colorAttachment = _colorAttachments[i];

            if (colorAttachment->GetSampleCount() != _renderPassLayout->GetSampleCount()) {
                LOG_ERROR("Color attachment sample count does not match render pass layout sample count.");
                return false;
            }

			vk::ImageView view = GetViewFromAttachment(colorAttachment);
			if (!view) {
                LOG_ERROR("Failed to get image view from color attachment.");
				return false;
			}

			attachmentViews[i] = view;
        }

        if (_depthStencilAttachment) {
            if (_depthStencilAttachment->GetSampleCount() != _renderPassLayout->GetSampleCount()) {
                LOG_ERROR("Depth stencil attachment sample count does not match render pass layout sample count.");
                return false;
            }

			vk::ImageView view = GetViewFromAttachment(_depthStencilAttachment);
            if (!view) {
                LOG_ERROR("Failed to get image view from depth stencil attachment.");
                return false;
            }

			attachmentViews.push_back(view);
        }

        if (_resolveAttachment) {
			vk::ImageView view = GetViewFromAttachment(_resolveAttachment);
			if (!view) {
				LOG_ERROR("Failed to get image view from resolve attachment.");
				return false;
			}

            attachmentViews.push_back(view);
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

	vk::ImageView VkFramebuffer::GetViewFromAttachment(Ref<Texture> attachment) {
		if (attachment == nullptr) {
			return nullptr;
		}

		auto vkTexture2D = std::dynamic_pointer_cast<VkTexture2D>(attachment);
		if (vkTexture2D) {
			return vkTexture2D->GetVkImageView();
		}
		auto vkTextureCube = std::dynamic_pointer_cast<VkTextureCube>(attachment);
		if (vkTextureCube) {
			return vkTextureCube->GetVkImageView();
		}
		auto vkTexture2DArray = std::dynamic_pointer_cast<VkTexture2DArray>(attachment);
		if (vkTexture2DArray) {
			return vkTexture2DArray->GetVkImageView();
		}

		return nullptr;
	}
}

#endif