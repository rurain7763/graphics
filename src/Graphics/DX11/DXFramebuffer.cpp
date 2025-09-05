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
		, _attachments(descriptor.attachments)
		, _resizeHandler(descriptor.resizeHandler)
	{
		if (!_renderPass) {
			LOG_FATAL("DXFramebuffer: Render pass layout is null.");
			return;
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

		_width = width;
		_height = height;

		_resizeHandler(width, height, _attachments);

		for (auto& attachment : _attachments) {
			if (attachment->GetWidth() != _width || attachment->GetHeight() != _height) {
				LOG_FATAL("DXFramebuffer: Attachment size does not match framebuffer size after resize.");
				_attachments.clear();
				return;
			}
		}
	}
}

#endif