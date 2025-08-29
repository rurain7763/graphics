#include "pch.h"
#include "DXBuffers.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXIndexBuffer::DXIndexBuffer(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _bufferByteSize(descriptor.bufferSize)
		, _memProperty(descriptor.memProperty)
		, _indexCount(descriptor.bufferSize / sizeof(uint32_t))
	{
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.ByteWidth = _bufferByteSize;
		bufferDesc.Usage = ConvertToDXUsage(_memProperty);
		bufferDesc.CPUAccessFlags = ConvertToDXCPUAccessFlags(_memProperty);
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		_nativeBuffer = DXNativeBuffer::Create(context, bufferDesc, descriptor.initialData);
	}

	void DXIndexBuffer::Update(const uint32_t* indices, uint32_t count) {
		if (_memProperty == MemoryProperty::Static) {
			
			return;
		}

		if (sizeof(uint32_t) * count > _bufferByteSize) {
			LOG_ERROR("Index count exceeds buffer size");
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (FAILED(_context.DeviceContext()->Map(_nativeBuffer.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			LOG_ERROR("Failed to map buffer");
		}
		else {
			memcpy(mappedResource.pData, indices, sizeof(uint32_t) * count);
			_context.DeviceContext()->Unmap(_nativeBuffer.buffer.Get(), 0);
		}

		_indexCount = count;
	}
}

#endif