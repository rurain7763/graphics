#include "pch.h"
#include "DXBuffers.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"

namespace flaw {
	DXNativeBuffer DXNativeBuffer::Create(DXContext& context, const D3D11_BUFFER_DESC& descriptor, const void* initialData) {
		DXNativeBuffer result;

		if (!initialData) {
			if (FAILED(context.Device()->CreateBuffer(&descriptor, nullptr, result.buffer.GetAddressOf()))) {
				throw std::runtime_error("Buffer Creation failed");
			}
		}
		else {
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = initialData;

			if (FAILED(context.Device()->CreateBuffer(&descriptor, &initData, result.buffer.GetAddressOf()))) {
				throw std::runtime_error("Buffer Creation failed");
			}
		}

		return result;
	}
}

#endif