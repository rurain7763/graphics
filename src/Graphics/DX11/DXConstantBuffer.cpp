#include "pch.h"
#include "DXBuffers.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXConstantBuffer::DXConstantBuffer(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _memProperty(descriptor.memProperty)
		, _bufferByteSize(descriptor.bufferSize)
	{
		FASSERT(_bufferByteSize % 16 == 0 && _bufferByteSize <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16);

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.ByteWidth = _bufferByteSize;
		bufferDesc.Usage = ConvertToDXUsage(_memProperty);
		bufferDesc.CPUAccessFlags = ConvertToDXCPUAccessFlags(_memProperty);
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		_nativeBuffer = DXNativeBuffer::Create(context, bufferDesc, descriptor.initialData);
	}

	void DXConstantBuffer::Update(const void* data, int32_t size) {
		if (_memProperty == MemoryProperty::Static) {
			LOG_ERROR("Cannot update static constant buffer");
			return;
		}

		if (size > _bufferByteSize) {
			LOG_ERROR("Constant buffer size exceeds allocated size");
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (FAILED(_context.DeviceContext()->Map(_nativeBuffer.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			LOG_ERROR("Failed to map constant buffer");
			return;
		}

		memcpy(mappedResource.pData, data, size);

		_context.DeviceContext()->Unmap(_nativeBuffer.buffer.Get(), 0);
	}
}

#endif