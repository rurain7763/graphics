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
		, _width(descriptor.width)
		, _height(descriptor.height)
		, _colorAttachments(descriptor.colorAttachments)
		, _colorResizeHandler(descriptor.colorResizeHandler)
		, _renderPassLayout(std::static_pointer_cast<DXRenderPassLayout>(descriptor.renderPassLayout))
	{
		if (!_renderPassLayout) {
			LOG_FATAL("DXFramebuffer: Render pass layout is null.");
			return;
		}

		if (descriptor.depthStencilAttachment) {
			_depthStencilAttachment = descriptor.depthStencilAttachment.value();
			_depthStencilResizeHandler = descriptor.depthStencilResizeHandler;
		}

		if (descriptor.resolveAttachment) {
			_resolveAttachment = descriptor.resolveAttachment.value();
			_resolveResizeHandler = descriptor.resolveResizeHandler;
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

		_width = width;
		_height = height;
	}

	Ref<RenderPassLayout> DXFramebuffer::GetRenderPassLayout() const {
		return _renderPassLayout;
	}
}

#endif