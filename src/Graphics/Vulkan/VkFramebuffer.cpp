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
		, _width(descriptor.width)
		, _height(descriptor.height)
		, _layers(descriptor.layers)
        , _renderPass(std::static_pointer_cast<VkRenderPass>(descriptor.renderPass))
		, _attachments(descriptor.attachments)
		, _resizeHandler(descriptor.resizeHandler)
    {
        if (_renderPass == nullptr) {
            Log::Fatal("Render pass layout cannot be null for VkFramebuffer.");
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
        if (width == _width && height == _height) {
            return;
        }

        if (!_resizeHandler) {
            return;
        }

		_resizeHandler(width, height, _attachments);

		_width = _attachments[0]->GetWidth();
		_height = _attachments[0]->GetHeight();

        _context.AddDelayedDeletionTasks([&context = _context, framebuffer = _framebuffer]() {
            context.GetVkDevice().destroyFramebuffer(framebuffer, nullptr);
        });

        if (!CreateFramebuffer()) {
            LOG_FATAL("Failed to resize Vulkan framebuffer.");
        }
    }

    bool VkFramebuffer::CreateFramebuffer() {
        std::vector<vk::ImageView> attachmentViews;
		for (auto& attachment : _attachments) {
			if (attachment->GetWidth() != _width || attachment->GetHeight() != _height) {
				LOG_ERROR("Attachment size does not match framebuffer size.");
				return false;
			}

			if (attachment->GetLayers() != _layers) {
				LOG_ERROR("Attachment layers does not match framebuffer layers.");
				return false;
			}

			const auto& vkNativeTexView = static_cast<const VkNativeTextureView&>(attachment->GetNativeTextureView());

			attachmentViews.push_back(vkNativeTexView.imageView);
		}

        vk::FramebufferCreateInfo createInfo;
        createInfo.renderPass = _renderPass->GetNativeVkRenderPass();
        createInfo.attachmentCount = attachmentViews.size();
        createInfo.pAttachments = attachmentViews.data();
        createInfo.width = _width;
        createInfo.height = _height;
		createInfo.layers = _layers;
        
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