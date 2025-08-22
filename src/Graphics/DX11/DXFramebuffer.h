#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsFramebuffer.h"

namespace flaw {
	class DXContext;
	class DXRenderPassLayout;

	class DXFramebuffer : public GraphicsFramebuffer {
	public:
		DXFramebuffer(DXContext& context, const Descriptor& descriptor);
		~DXFramebuffer();

		void Resize(uint32_t width, uint32_t height) override;

		inline Ref<Texture> GetAttachment(uint32_t index) const override { return _colorAttachments[index]; }
		inline Ref<Texture> GetDepthStencilAttachment() const override { return _depthStencilAttachment; }
		inline Ref<Texture> GetResolveAttachment() const override { return _resolveAttachment; }

		Ref<RenderPassLayout> GetRenderPassLayout() const override;

		inline uint32_t GetWidth() const override { return _width; }
		inline uint32_t GetHeight() const override { return _height; }

	private:
		DXContext& _context;

		uint32_t _width, _height;

		std::vector<Ref<Texture>> _colorAttachments;
		Ref<Texture> _depthStencilAttachment;
		Ref<Texture> _resolveAttachment;

		Ref<DXRenderPassLayout> _renderPassLayout;

		std::function<bool(Ref<Texture>&, uint32_t, uint32_t)> _colorResizeHandler;
		std::function<bool(Ref<Texture>&, uint32_t, uint32_t)> _depthStencilResizeHandler;
		std::function<bool(Ref<Texture>&, uint32_t, uint32_t)> _resolveResizeHandler;
	};
}

#endif