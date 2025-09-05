#pragma once

#include "Core.h"

#ifdef SUPPORT_DX11

#include "DXCore.h"
#include "Graphics/GraphicsFramebuffer.h"

namespace flaw {
	class DXContext;
	class DXRenderPass;

	class DXFramebuffer : public Framebuffer {
	public:
		DXFramebuffer(DXContext& context, const Descriptor& descriptor);
		~DXFramebuffer();

		void Resize(uint32_t width, uint32_t height) override;

		inline uint32_t GetAttachmentCount() const override { return _attachments.size(); }
		inline Ref<Texture> GetAttachment(uint32_t index) const override { return _attachments.at(index); }

		inline uint32_t GetWidth() const override { return _width; }
		inline uint32_t GetHeight() const override { return _height; }

	private:
		DXContext& _context;

		Ref<DXRenderPass> _renderPass;

		uint32_t _width, _height;

		std::vector<Ref<Texture>> _attachments;
		std::function<void(uint32_t, uint32_t, std::vector<Ref<Texture>>&)> _resizeHandler;
	};
}

#endif