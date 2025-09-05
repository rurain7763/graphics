#pragma once

#include "GraphicsContext.h"

#include <vector>
#include <functional>

namespace flaw {
	template <typename T>
	class GraphicsResourcesPool {
	public:
		GraphicsResourcesPool(GraphicsContext& context, const std::function<Ref<T>(GraphicsContext&)>& factory)
			: _context(context)
			, _factory(factory)
			, _used(0)
		{
			_resourcesPerFrame.resize(_context.GetFrameCount());
		}

		Ref<T> Get() {
			uint32_t frameIndex = _context.GetCurrentFrameIndex();
			if (_used >= _resourcesPerFrame[frameIndex].size()) {
				auto resource = _factory(_context);
				_resourcesPerFrame[frameIndex].push_back(resource);
			}
			return _resourcesPerFrame[frameIndex][_used++];
		}

		void Reset() {
			_used = 0;
		}

	private:
		GraphicsContext& _context;
		std::function<Ref<T>(GraphicsContext&)> _factory;
		std::vector<std::vector<Ref<T>>> _resourcesPerFrame;
		uint32_t _used;
	};

	class FramebufferGroup {
	public:
		FramebufferGroup(GraphicsContext& context, const std::function<Ref<Framebuffer>(GraphicsContext&, uint32_t)>& desc)
			: _context(context)
			, _framebuffersPerFrame(context.GetFrameCount())
		{
			for (uint32_t i = 0; i < context.GetFrameCount(); i++) {
				_framebuffersPerFrame[i] = desc(context, i);
			}
		}

		Ref<Framebuffer> Get() {
			uint32_t frameIndex = _context.GetCurrentFrameIndex();
			return _framebuffersPerFrame[frameIndex];
		}

	private:
		GraphicsContext& _context;

		std::vector<Ref<Framebuffer>> _framebuffersPerFrame;
	};
}