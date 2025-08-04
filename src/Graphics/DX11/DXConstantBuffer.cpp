#include "pch.h"
#include "DXConstantBuffer.h"
#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXConstantBuffer::DXConstantBuffer(DXContext& context, uint32_t size)
		: _context(context)
	{
		Initialize(size);
	}

	void DXConstantBuffer::Initialize(uint32_t size) {
		assert(size % 16 == 0 && size <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16);

		_size = size;
		
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.ByteWidth = size;

		// set buffer data write dynamically or not 
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

		bufferDesc.MiscFlags = 0;

		if (FAILED(_context.Device()->CreateBuffer(&bufferDesc, nullptr, _buffer.GetAddressOf()))) {
			throw std::exception("CreateBuffer failed");
		}
	}

	void DXConstantBuffer::Update(const void* data, int32_t size) {
		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		_context.DeviceContext()->Map(_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		memcpy(mappedResource.pData, data, size);
		_context.DeviceContext()->Unmap(_buffer.Get(), 0);
	}

	void DXConstantBuffer::BindToGraphicsShader(const uint32_t slot) {
		_context.DeviceContext()->VSSetConstantBuffers(slot, 1, _buffer.GetAddressOf());
		_context.DeviceContext()->PSSetConstantBuffers(slot, 1, _buffer.GetAddressOf());
		_context.DeviceContext()->GSSetConstantBuffers(slot, 1, _buffer.GetAddressOf());
		_context.DeviceContext()->HSSetConstantBuffers(slot, 1, _buffer.GetAddressOf());
		_context.DeviceContext()->DSSetConstantBuffers(slot, 1, _buffer.GetAddressOf());

		_unbindFunc = [this, slot]() {
			ID3D11Buffer* nullBuffer = nullptr;
			_context.DeviceContext()->VSSetConstantBuffers(slot, 1, &nullBuffer);
			_context.DeviceContext()->PSSetConstantBuffers(slot, 1, &nullBuffer);
			_context.DeviceContext()->GSSetConstantBuffers(slot, 1, &nullBuffer);
			_context.DeviceContext()->HSSetConstantBuffers(slot, 1, &nullBuffer);
			_context.DeviceContext()->DSSetConstantBuffers(slot, 1, &nullBuffer);

			_unbindFunc = nullptr;
		};
	}

	void DXConstantBuffer::BindToComputeShader(const uint32_t slot) {
		_context.DeviceContext()->CSSetConstantBuffers(slot, 1, _buffer.GetAddressOf());

		_unbindFunc = [this, slot]() {
			ID3D11Buffer* nullBuffer = nullptr;
			_context.DeviceContext()->CSSetConstantBuffers(slot, 1, &nullBuffer);

			_unbindFunc = nullptr;
		};
	}

	void DXConstantBuffer::Unbind() {
		if (_unbindFunc) {
			_unbindFunc();
		}
	}
}