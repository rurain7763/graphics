#include "pch.h"
#include "DXFramebuffer.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXRenderPass.h"
#include "DXTextures.h"
#include "Log/Log.h"

namespace flaw {
	DXFramebuffer::DXFramebuffer(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _renderPass(std::static_pointer_cast<DXRenderPass>(descriptor.renderPass))
		, _width(descriptor.width)
		, _height(descriptor.height)
		, _layers(descriptor.layers)
		, _attachments(descriptor.attachments)
		, _resizeHandler(descriptor.resizeHandler)
	{
		if (!_renderPass) {
			LOG_FATAL("Render pass layout is null.");
			return;
		}

		for (auto& attachment : _attachments) {
			if (attachment->GetWidth() != _width || attachment->GetHeight() != _height) {
				LOG_FATAL("Attachment size does not match framebuffer size.");
				return;
			}

			if (attachment->GetLayers() != _layers) {
				LOG_FATAL("Attachment layers does not match framebuffer layers.");
				return;
			}
		}

		_context.AddOnResizeHandler(PID(this), std::bind(&DXFramebuffer::Resize, this, std::placeholders::_1, std::placeholders::_2));
	}

	DXFramebuffer::~DXFramebuffer() {
		_context.RemoveOnResizeHandler(PID(this));
	}

	void DXFramebuffer::Resize(uint32_t width, uint32_t height) {
		if (width == _width && height == _height) {
			return;
		}

		if (!_resizeHandler) {
			return;
		}

		_resizeHandler(width, height, _attachments);

		_width = _attachments[0]->GetWidth();
		_height = _attachments[0]->GetHeight();

		for (auto& attachment : _attachments) {
			if (attachment->GetWidth() != _width || attachment->GetHeight() != _height) {
				LOG_FATAL("Attachment size does not match framebuffer size after resize.");
				_attachments.clear();
				return;
			}

			if (attachment->GetLayers() != _layers) {
				LOG_FATAL("Attachment layers does not match framebuffer layers after resize.");
				_attachments.clear();
				return;
			}
		}
	}
}

#endif