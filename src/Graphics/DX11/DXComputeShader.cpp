#include "pch.h"
#include "DXShaders.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXComputeShader::DXComputeShader(DXContext& context, const Descriptor& descriptor)
		:_context(context)
	{
		if (!CreateComputeShader(descriptor.file.c_str(), descriptor.entry.c_str())) {
			Log::Error("CreateComputeShader failed");
			return;
		}
	}

	bool DXComputeShader::CreateComputeShader(const char* filepath, const char* entryPoint) {
		std::wstring wfilename = std::wstring(filepath, filepath + strlen(filepath));
		ComPtr<ID3DBlob> errorBlob = nullptr;

		if (FAILED(D3DCompileFromFile(
			wfilename.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entryPoint,
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

#endif