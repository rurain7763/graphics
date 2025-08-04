#include "pch.h"
#include "DXComputeShader.h"
#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXComputeShader::DXComputeShader(DXContext& context, const char* filepath)
		:_context(context)
	{
		if (!CreateComputeShader(filepath)) {
			Log::Error("CreateComputeShader failed");
			return;
		}
	}

	void DXComputeShader::Bind() {
		_context.DeviceContext()->CSSetShader(_computeShader.Get(), nullptr, 0);
	}

	void DXComputeShader::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
		// x * y * z = thread group count
		_context.DeviceContext()->Dispatch(x, y, z);
	}

	bool DXComputeShader::CreateComputeShader(const char* filepath) {
		std::wstring wfilename = std::wstring(filepath, filepath + strlen(filepath));
		ComPtr<ID3DBlob> errorBlob = nullptr;

		if (FAILED(D3DCompileFromFile(
			wfilename.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"CSMain",
			"cs_5_0",
			D3DCOMPILE_DEBUG,
			0,
			_csBlob.GetAddressOf(),
			errorBlob.GetAddressOf()
		))) {
			if (errorBlob) {
				Log::Error("%s", (char*)errorBlob->GetBufferPointer());
			}
			else {
				Log::Error("D3DCompileFromFile failed");
			}
			return false;
		}

		if (FAILED(_context.Device()->CreateComputeShader(_csBlob->GetBufferPointer(), _csBlob->GetBufferSize(), nullptr, _computeShader.GetAddressOf()))) {
			return false;
		}

		return true;
	}
}