#include "pch.h"
#include "DXStructuredBuffer.h"
#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXStructuredBuffer::DXStructuredBuffer(DXContext& context, const Descriptor& desc)
		: _context(context)
	{
		Create(desc);
	}

	void DXStructuredBuffer::Create(const Descriptor& desc) {
		_buffer.Reset();
		_writeOnlyBuffer.Reset();
		_readOnlyBuffer.Reset();

		_srv.Reset();
		_uav.Reset();

		_buffer = CreateBuffer(desc.elmSize, desc.count, GetBindFlag(desc.bindFlags), (AccessFlag)0, desc.initialData);

		if (desc.bindFlags & static_cast<uint32_t>(BindFlag::ShaderResource)) {
			_srv = CreateShaderResourceView(desc.count);
		}

		if (desc.bindFlags & static_cast<uint32_t>(BindFlag::UnorderedAccess)) {
			_uav = CreateUnorderedAccessView(desc.count);
		}

		if (desc.accessFlags & AccessFlag::Write) {
			_writeOnlyBuffer = CreateBuffer(desc.elmSize, desc.count, GetBindFlag(BindFlag::ShaderResource), AccessFlag::Write, nullptr);
		}

		if (desc.accessFlags & AccessFlag::Read) {
			_readOnlyBuffer = CreateBuffer(desc.elmSize, desc.count, (BindFlag)0, AccessFlag::Read, nullptr);
		}

		_size = desc.elmSize * desc.count;
	}

	void DXStructuredBuffer::Update(const void* data, uint32_t size) {
		assert(size <= _size || _writeOnlyBuffer);

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (FAILED(_context.DeviceContext()->Map(_writeOnlyBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
			Log::Warn("StructuredBuffer::Update: Map failed");
			return;
		}
		else {
			memcpy(mappedResource.pData, data, size);
			_context.DeviceContext()->Unmap(_writeOnlyBuffer.Get(), 0);
			_context.DeviceContext()->CopyResource(_buffer.Get(), _writeOnlyBuffer.Get());
		}
	}

	void DXStructuredBuffer::Fetch(void* data, uint32_t size) {
		assert(size <= _size || _readOnlyBuffer);

		D3D11_BOX box = {};
		box.left = 0;
		box.top = 0;
		box.front = 0;
		box.right = size;
		box.bottom = 1;
		box.back = 1;

		_context.DeviceContext()->CopySubresourceRegion(
			_readOnlyBuffer.Get(), 
			0, 
			0, 0, 0, 
			_buffer.Get(), 
			0, 
			&box
		);

		D3D11_MAPPED_SUBRESOURCE mappedResource = {};
		if (FAILED(_context.DeviceContext()->Map(_readOnlyBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedResource))) {
			Log::Warn("StructuredBuffer::Get: Map failed");
			return;
		}
		else {
			memcpy(data, mappedResource.pData, size);
			_context.DeviceContext()->Unmap(_readOnlyBuffer.Get(), 0);
		}
	}

	ComPtr<ID3D11Buffer> DXStructuredBuffer::CreateBuffer(uint32_t elmSize, uint32_t count, uint32_t bindFlag, AccessFlag usage, const void* initData) {
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = bindFlag;
		bufferDesc.ByteWidth = elmSize * count;

		switch (usage)
		{
		case AccessFlag::Write:
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			break;
		case AccessFlag::Read:
			bufferDesc.Usage = D3D11_USAGE_STAGING;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			break;
		default:
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.CPUAccessFlags = 0;
			break;
		}

		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.StructureByteStride = elmSize;

		ComPtr<ID3D11Buffer> buffer;

		if (initData) {
			D3D11_SUBRESOURCE_DATA subResourceData = {};
			subResourceData.pSysMem = initData;

			if (FAILED(_context.Device()->CreateBuffer(&bufferDesc, &subResourceData, buffer.GetAddressOf()))) {
				throw std::exception("CreateBuffer failed");
			}
			return buffer;
		}
		else {
			if (FAILED(_context.Device()->CreateBuffer(&bufferDesc, nullptr, buffer.GetAddressOf()))) {
				throw std::exception("CreateBuffer failed");
			}
		}

		return buffer;
	}

	ComPtr<ID3D11ShaderResourceView> DXStructuredBuffer::CreateShaderResourceView(uint32_t count) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = count;

		ComPtr<ID3D11ShaderResourceView> srv;
		if (FAILED(_context.Device()->CreateShaderResourceView(_buffer.Get(), &srvDesc, srv.GetAddressOf()))) {
			throw std::exception("CreateShaderResourceView failed");
		}

		return srv;
	}

	ComPtr<ID3D11UnorderedAccessView> DXStructuredBuffer::CreateUnorderedAccessView(uint32_t count) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = count;
		uavDesc.Buffer.Flags = 0;

		ComPtr<ID3D11UnorderedAccessView> uav;
		if (FAILED(_context.Device()->CreateUnorderedAccessView(_buffer.Get(), &uavDesc, uav.GetAddressOf()))) {
			throw std::exception("CreateUnorderedAccessView failed");
		}

		return uav;
	}

	uint32_t DXStructuredBuffer::GetBindFlag(uint32_t bindFlag) {
		uint32_t flag = 0;

		if (bindFlag & static_cast<uint32_t>(BindFlag::ShaderResource)) {
			flag |= D3D11_BIND_SHADER_RESOURCE;
		}

		if (bindFlag & static_cast<uint32_t>(BindFlag::UnorderedAccess)) {
			flag |= D3D11_BIND_UNORDERED_ACCESS;
		}

		return flag;
	}
}