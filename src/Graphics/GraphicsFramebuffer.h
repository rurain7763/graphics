#pragma once

#include "Core.h"
#include "GraphicsRenderPass.h"
#include "GraphicsTextures.h"

#include <optional>
#include <functional>

namespace flaw {
	class Framebuffer {
	public:
        struct Descriptor {
            Ref<RenderPass> renderPass;
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t layers = 1;
            std::vector<Ref<Texture>> attachments;
			std::function<void(uint32_t width, uint32_t height, std::vector<Ref<Texture>>& attachments)> resizeHandler;
        };

		Framebuffer() = default;
		virtual ~Framebuffer() = default;

        virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual uint32_t GetAttachmentCount() const = 0;
		virtual Ref<Texture> GetAttachment(uint32_t index) const = 0;
        
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetLayers() const = 0;
	};
}