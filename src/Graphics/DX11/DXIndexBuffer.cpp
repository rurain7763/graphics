#include "pch.h"
#include "DXIndexBuffer.h"

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXIndexBuffer::DXIndexBuffer(DXContext& context, const Descriptor& descriptor)
		: _context(context)
	{
		CreateBuffer(descriptor);
	}

	void DXIndexBuffer::Update(const uint32_t* indices, uint32_t count) {
		if (_usage == UsageFlag::Static || _usage == UsageFlag::Staging) {
			Log::Error("Cannot update static or staging buffer");
			return;
		}

		if (sizeof(uint32_t) * count > _size) {
			Log::Error("Buffer size is too small");
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		if (FAILED(_context.DeviceContext()->Map(_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			Log::Error("Failed to map buffer");
		}
		else {
			memcpy(mappedResource.pData, indices, sizeof(uint32_t) * count);
			_context.DeviceContext()->Unmap(_buffer.Get(), 0);
		}
	}

	void DXIndexBuffer::Bind() {
		_context.DeviceContext()->IASetIndexBuffer(_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

	void DXIndexBuffer::CreateBuffer(const Descriptor& desc) {
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
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
				Log::Error("Buffer Creation failed");
			}
		}

		_usage = desc.usage;
		_size = desc.bufferSize;
		_indexCount = desc.bufferSize / sizeof(uint32_t);
	}
}