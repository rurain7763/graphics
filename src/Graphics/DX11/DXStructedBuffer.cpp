#include "pch.h"
#include "DXBuffers.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXStructuredBuffer::DXStructuredBuffer(DXContext& context, const Descriptor& desc)
		: _context(context)
		, _memProperty(desc.memProperty)
		, _usages(desc.bufferUsages)
		, _elmSize(desc.elmSize)
		, _bufferByteSize(desc.bufferSize)
	{
		if (!CreateBuffer(desc.initialData)) {
			return;
		}

		if (desc.bufferUsages & BufferUsage::ShaderResource) {
			CreateShaderResourceView();
		}

		if (desc.bufferUsages & BufferUsage::UnorderedAccess) {
			CreateUnorderedAccessView();
		}
	}

	bool DXStructuredBuffer::CreateBuffer(const void* data) {
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = ConvertToDXBufferBindFlags(_usages);
		bufferDesc.ByteWidth = _bufferByteSize;
		bufferDesc.Usage = ConvertToDXUsage(_memProperty);
		bufferDesc.CPUAccessFlags = ConvertToDXCPUAccessFlags(_memProperty);
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = _elmSize;

		if (data) {
			D3D11_SUBRESOURCE_DATA subResourceData = {};
			subResourceData.pSysMem = data;

			if (FAILED(_context.Device()->CreateBuffer(&bufferDesc, &subResourceData, _buffer.GetAddressOf()))) {
				LOG_ERROR("CreateBuffer failed with initial data");
				return false;
			}
		}
		else {
			if (FAILED(_context.Device()->CreateBuffer(&bufferDesc, nullptr, _buffer.GetAddressOf()))) {
				LOG_ERROR("CreateBuffer failed");
				return false;
			}
		}

		return true;
	}

	void DXStructuredBuffer::CreateShaderResourceView() {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = _bufferByteSize / _elmSize;

		if (FAILED(_context.Device()->CreateShaderResourceView(_buffer.Get(), &srvDesc, _srv.GetAddressOf()))) {
			LOG_ERROR("CreateShaderResourceView failed");
		}
	}

	void DXStructuredBuffer::CreateUnorderedAccessView() {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = _bufferByteSize / _elmSize;
		uavDesc.Buffer.Flags = 0;

		if (FAILED(_context.Device()->CreateUnorderedAccessView(_buffer.Get(), &uavDesc, _uav.GetAddressOf()))) {
			LOG_ERROR("CreateUnorderedAccessView failed");
		}
	}

	void DXStructuredBuffer::Update(const void* data, uint32_t size) {
		if (_memProperty == MemoryProperty::Static) {
			LOG_ERROR("Cannot update a static buffer");
			return;
		}

		if (!data || size == 0 || size > _bufferByteSize) {
			LOG_ERROR("Invalid data or size");
			return;
		}

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (FAILED(_context.DeviceContext()->Map(_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			LOG_ERROR("Failed to map buffer for update");
			return;
		}
		memcpy(mappedResource.pData, data, size);
		_context.DeviceContext()->Unmap(_buffer.Get(), 0);
	}

	void DXStructuredBuffer::Fetch(void* data, uint32_t size) {
		if (_memProperty != MemoryProperty::Staging) {
			LOG_ERROR("Fetch is only supported for staging buffers");
			return;
		}

		FASSERT(size <= _bufferByteSize);

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (FAILED(_context.DeviceContext()->Map(_buffer.Get(), 0, D3D11_MAP_READ, 0, &mappedResource))) {
			LOG_ERROR("Failed to map buffer for reading");
			return;
		}

		memcpy(data, mappedResource.pData, size);
		_context.DeviceContext()->Unmap(_buffer.Get(), 0);
	}

	void DXStructuredBuffer::CopyTo(Ref<StructuredBuffer> dstBuffer, uint32_t srcOffset, uint32_t dstOffset) {
		auto dxDstBuffer = std::static_pointer_cast<DXStructuredBuffer>(dstBuffer);
		FASSERT(dxDstBuffer, "Destination buffer is not a DXStructuredBuffer");

		D3D11_BOX srcBox = {};
		srcBox.left = srcOffset;
		srcBox.right = _bufferByteSize;
		srcBox.top = 0;
		srcBox.bottom = 1;
		srcBox.front = 0;
		srcBox.back = 1;

		_context.DeviceContext()->CopySubresourceRegion(
			dxDstBuffer->_buffer.Get(),
			0,
			dstOffset,
			0,
			0,
			_buffer.Get(),
			0,
			&srcBox
		);
	}

	void DXStructuredBuffer::CopyFrom(Ref<StructuredBuffer> srcBuffer, uint32_t srcOffset, uint32_t dstOffset) {
		auto dxSrcBuffer = std::static_pointer_cast<DXStructuredBuffer>(srcBuffer);
		FASSERT(dxSrcBuffer, "Source buffer is not a DXStructuredBuffer");

		D3D11_BOX srcBox = {};
		srcBox.left = srcOffset;
		srcBox.right = dxSrcBuffer->_bufferByteSize;
		srcBox.top = 0;
		srcBox.bottom = 1;
		srcBox.front = 0;
		srcBox.back = 1;

		_context.DeviceContext()->CopySubresourceRegion(
			_buffer.Get(),
			0,
			dstOffset,
			0,
			0,
			dxSrcBuffer->_buffer.Get(),
			0,
			&srcBox
		);
	}
}

#endif