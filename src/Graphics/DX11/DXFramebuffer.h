#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsFramebuffer.h"

namespace flaw {
	class DXContext;
	class DXRenderPassLayout;

	class DXFramebuffer : public Framebuffer {
	public:
		DXFramebuffer(DXContext& context, const Descriptor& descriptor);
		~DXFramebuffer();

		void Resize(uint32_t width, uint32_t height) override;

		inline uint32_t GetColorAttachmentCount() const override { return _colorAttachments.size(); }
		inline Ref<Texture> GetColorAttachment(uint32_t index) const override { return _colorAttachments[index]; }
		inline Ref<Texture> GetDepthStencilAttachment() const override { return _depthStencilAttachment; }
		inline uint32_t GetResolveAttachmentCount() const override { return _resolveAttachments.size(); }
		inline Ref<Texture> GetResolveAttachment(uint32_t index) const override { return _resolveAttachments[index]; }

		Ref<RenderPassLayout> GetRenderPassLayout() const override;

		inline uint32_t GetWidth() const override { return _width; }
		inline uint32_t GetHeight() const override { return _height; }

	private:
		DXContext& _context;

		uint32_t _width, _height;

		std::vector<Ref<Texture>> _colorAttachments;
		Ref<Texture> _depthStencilAttachment;
		std::vector<Ref<Texture>> _resolveAttachments;

		Ref<DXRenderPassLayout> _renderPassLayout;

		std::function<bool(Ref<Texture>&, uint32_t, uint32_t)> _colorResizeHandler;
		std::function<bool(Ref<Texture>&, uint32_t, uint32_t)> _depthStencilResizeHandler;
		std::function<bool(Ref<Texture>&, uint32_t, uint32_t)> _resolveResizeHandler;
	};
}

#endif