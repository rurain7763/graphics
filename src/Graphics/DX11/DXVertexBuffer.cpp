#include "pch.h"
#include "DXBuffers.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXVertexBuffer::DXVertexBuffer(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _memProperty(descriptor.memProperty)
		, _elmSize(descriptor.elmSize)
		, _bufferByteSize(descriptor.bufferSize)
	{
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = _bufferByteSize;
		bufferDesc.Usage = ConvertToDXUsage(_memProperty);
		bufferDesc.CPUAccessFlags = ConvertToDXCPUAccessFlags(_memProperty);
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		_nativeBuffer = DXNativeBuffer::Create(context, bufferDesc, descriptor.initialData);
	}

	void DXVertexBuffer::Update(const void* data, uint32_t size) {
		if (_memProperty == MemoryProperty::Static) {
			LOG_ERROR("Cannot update static vertex buffer");
			return;
		}

		if (size > _bufferByteSize) {
			LOG_ERROR("Data size exceeds buffer size: %d > %d", size, _bufferByteSize);
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (FAILED(_context.DeviceContext()->Map(_nativeBuffer.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			Log::Error("Failed to map buffer");
		}
		else {
			memcpy(mappedResource.pData, data, size);
			_context.DeviceContext()->Unmap(_nativeBuffer.buffer.Get(), 0);
		}
	}

	void DXVertexBuffer::CopyTo(Ref<VertexBuffer> dstBuffer, uint32_t srcOffset, uint32_t dstOffset) {
		auto dxDstBuffer = std::static_pointer_cast<DXVertexBuffer>(dstBuffer);
		FASSERT(dxDstBuffer, "Destination buffer is not a DXVertexBuffer");

		if (dstBuffer->Size() < _bufferByteSize) {
			LOG_ERROR("Destination buffer size is smaller than source buffer size");
			return;
		}

		D3D11_BOX srcBox = {};
		srcBox.left = srcOffset;
		srcBox.right = _bufferByteSize;
		srcBox.top = 0;
		srcBox.bottom = 1;
		srcBox.front = 0;
		srcBox.back = 1;

		_context.DeviceContext()->CopySubresourceRegion(
			dxDstBuffer->_nativeBuffer.buffer.Get(),
			0,
			dstOffset,
			0,
			0,
			_nativeBuffer.buffer.Get(),
			0,
			&srcBox
		);
	}
}

#endif