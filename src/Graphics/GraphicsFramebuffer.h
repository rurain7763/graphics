#pragma once

#include "Core.h"
#include "GraphicsRenderPass.h"
#include "GraphicsTextures.h"

#include <optional>
#include <functional>

namespace flaw {
	class GraphicsFramebuffer {
	public:
        struct Descriptor {
            uint32_t width;
            uint32_t height;
            std::vector<Ref<Texture>> colorAttachments;
            std::function<bool(Ref<Texture>& tex, uint32_t width, uint32_t height)> colorResizeHandler;
            std::optional<Ref<Texture>> depthStencilAttachment;
            std::function<bool(Ref<Texture>& tex, uint32_t width, uint32_t height)> depthStencilResizeHandler;
            std::optional<Ref<Texture>> resolveAttachment;
            std::function<bool(Ref<Texture>& tex, uint32_t width, uint32_t height)> resolveResizeHandler;
            Ref<GraphicsRenderPassLayout> renderPassLayout;
        };

		GraphicsFramebuffer() = default;
		virtual ~GraphicsFramebuffer() = default;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual Ref<Texture> GetAttachment(uint32_t index) const = 0;
        virtual Ref<Texture> GetDepthStencilAttachment() const = 0;
        virtual Ref<Texture> GetResolveAttachment() const = 0;
        
        virtual Ref<GraphicsRenderPassLayout> GetRenderPassLayout() const = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
	};
}