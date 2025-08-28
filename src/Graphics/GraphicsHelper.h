#pragma once

#include "GraphicsContext.h"

#include <vector>

namespace flaw {
	class ShaderResourcesPool {
	public:
		ShaderResourcesPool(GraphicsContext& context, const ShaderResources::Descriptor& descriptor)
			: _context(context)
			, _descriptor(descriptor)
			, _used(0)
		{
			_shaderResourcesPerFrame.resize(_context.GetFrameCount());
		}

		Ref<ShaderResources> Get() {
			uint32_t frameIndex = _context.GetCurrentFrameIndex();
			if (_used >= _shaderResourcesPerFrame[frameIndex].size()) {
				auto shaderResources = _context.CreateShaderResources(_descriptor);
				_shaderResourcesPerFrame[frameIndex].push_back(shaderResources);
			}
			return _shaderResourcesPerFrame[frameIndex][_used++];
		}

		void Reset() {
			_used = 0;
		}

	private:
		GraphicsContext& _context;
		ShaderResources::Descriptor _descriptor;
		std::vector<std::vector<Ref<ShaderResources>>> _shaderResourcesPerFrame;
		uint32_t _used;
	};

	class ConstantBufferPool {
	public:
		ConstantBufferPool(GraphicsContext& context, const ConstantBuffer::Descriptor& descriptor)
			: _context(context)
			, _descriptor(descriptor)
			, _used(0)
		{
			_constantBuffersPerFrame.resize(_context.GetFrameCount());
		}

		Ref<ConstantBuffer> Get() {
			uint32_t frameIndex = _context.GetCurrentFrameIndex();
			if (_used >= _constantBuffersPerFrame[frameIndex].size()) {
				auto constantBuffer = _context.CreateConstantBuffer(_descriptor);
				_constantBuffersPerFrame[frameIndex].push_back(constantBuffer);
			}
			return _constantBuffersPerFrame[frameIndex][_used++];
		}

		void Reset() {
			_used = 0;
		};

	private:
		GraphicsContext& _context;
		ConstantBuffer::Descriptor _descriptor;
		std::vector<std::vector<Ref<ConstantBuffer>>> _constantBuffersPerFrame;
		uint32_t _used;
	};

	class StructuredBufferPool {
	public:
		StructuredBufferPool(GraphicsContext& context, const StructuredBuffer::Descriptor& descriptor)
			: _context(context)
			, _descriptor(descriptor)
			, _used(0)
		{
			_structuredBuffersPerFrame.resize(_context.GetFrameCount());
		}

		Ref<StructuredBuffer> Get() {
			uint32_t frameIndex = _context.GetCurrentFrameIndex();
			if (_used >= _structuredBuffersPerFrame[frameIndex].size()) {
				auto structuredBuffer = _context.CreateStructuredBuffer(_descriptor);
				_structuredBuffersPerFrame[frameIndex].push_back(structuredBuffer);
			}
			return _structuredBuffersPerFrame[frameIndex][_used++];
		}

		void Reset() {
			_used = 0;
		};

	private:
		GraphicsContext& _context;
		StructuredBuffer::Descriptor _descriptor;
		std::vector<std::vector<Ref<StructuredBuffer>>> _structuredBuffersPerFrame;
		uint32_t _used;
	};
}