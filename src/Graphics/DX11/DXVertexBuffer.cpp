#include "pch.h"
#include "DXVertexBuffer.h"

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXVertexBuffer::DXVertexBuffer(DXContext& context, const Descriptor& descriptor)
		: _context(context)
	{
		CreateBuffer(descriptor);
	}

	void DXVertexBuffer::Update(const void* data, uint32_t elmSize, uint32_t count) {
		if (_usage == UsageFlag::Static || _usage == UsageFlag::Staging) {
			Log::Error("Cannot update static or staging buffer");
			return;
		}

		if (elmSize * count > _size) {
			Log::Error("Buffer size is too small");
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (FAILED(_context.DeviceContext()->Map(_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			Log::Error("Failed to map buffer");
		}
		else {
			memcpy(mappedResource.pData, data, elmSize * count);
			_context.DeviceContext()->Unmap(_buffer.Get(), 0);
		}

		_elmSize = elmSize;
	}

	void DXVertexBuffer::Bind() {
		uint32_t offset = 0;
		_context.DeviceContext()->IASetVertexBuffers(0, 1, _buffer.GetAddressOf(), &_elmSize, &offset);
	}

	void DXVertexBuffer::CreateBuffer(const Descriptor& desc) {
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = desc.bufferSize;

		// set buffer data write dynamically or not 
		bufferDesc.CPUAccessFlags = desc.usage == UsageFlag::Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		bufferDesc.Usage = desc.usage == UsageFlag::Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;

		bufferDesc.MiscFlags = 0;

		if (!desc.initialData) {
			if (FAILED(_context.Device()->CreateBuffer(&bufferDesc, nullptr, _buffer.GetAddressOf()))) {
				Log::Error("Buffer Creation failed");
			}
		}
		else {
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = desc.initialData;
			if (FAILED(_context.Device()->CreateBuffer(&bufferDesc, &initData, _buffer.GetAddressOf()))) {
				Log::Error("Buffer Update failed");
			}
		}

		_usage = desc.usage;
		_elmSize = desc.elmSize;
		_size = desc.bufferSize;
	}
}
