#include "pch.h"
#include "DXShaders.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXGraphicsShader::DXGraphicsShader(DXContext& graphics, const Descriptor& desc)
		:_context(graphics)
	{
		bool success = true;

		if (!desc.vertexShaderFile.empty() && !CreateVertexShader(desc.vertexShaderFile.c_str(), desc.vertexShaderEntry.c_str())) {
			success = false;
		}

		if (!desc.hullShaderFile.empty() && !CreateHullShader(desc.hullShaderFile.c_str(), desc.hullShaderEntry.c_str())) {
			success = false;
		}

		if (!desc.domainShaderFile.empty() && !CreateDomainShader(desc.domainShaderFile.c_str(), desc.domainShaderEntry.c_str())) {
			success = false;
		}

		if (!desc.geometryShaderFile.empty() && !CreateGeometryShader(desc.geometryShaderFile.c_str(), desc.geometryShaderEntry.c_str())) {
			success = false;
		}

		if (!desc.pixelShaderFile.empty() && !CreatePixelShader(desc.pixelShaderFile.c_str(), desc.pixelShaderEntry.c_str())) {
			success = false;
		}

		if (!success) {
			Log::Error("Shader creation failed");

			_vsBlob.Reset();
			_vertexShader.Reset();

			_hsBlob.Reset();
			_hullShader.Reset();

			_dsBlob.Reset();
			_domainShader.Reset();

			_gsBlob.Reset();
			_geometryShader.Reset();

			_psBlob.Reset();
			_pixelShader.Reset();
		}
	}

	bool DXGraphicsShader::CreateVertexShader(const char* filePath, const char* entry) {
		if (!CompileShader(filePath, entry, "vs_5_0", _vsBlob)) {
			return false;
		}

		if (FAILED(_context.Device()->CreateVertexShader(_vsBlob->GetBufferPointer(), _vsBlob->GetBufferSize(), nullptr, _vertexShader.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXGraphicsShader::CreateHullShader(const char* filePath, const char* entry) {
		if (!CompileShader(filePath, entry, "hs_5_0", _hsBlob)) {
			return false;
		}

		if (FAILED(_context.Device()->CreateHullShader(_hsBlob->GetBufferPointer(), _hsBlob->GetBufferSize(), nullptr, _hullShader.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXGraphicsShader::CreateDomainShader(const char* filePath, const char* entry) {
		if (!CompileShader(filePath, entry, "ds_5_0", _dsBlob)) {
			return false;
		}
		if (FAILED(_context.Device()->CreateDomainShader(_dsBlob->GetBufferPointer(), _dsBlob->GetBufferSize(), nullptr, _domainShader.GetAddressOf()))) {
			return false;
		}
		return true;
	}

	bool DXGraphicsShader::CreateGeometryShader(const char* filePath, const char* entry) {
		if (!CompileShader(filePath, entry, "gs_5_0", _gsBlob)) {
			return false;
		}
		if (FAILED(_context.Device()->CreateGeometryShader(_gsBlob->GetBufferPointer(), _gsBlob->GetBufferSize(), nullptr, _geometryShader.GetAddressOf()))) {
			return false;
		}
		return true;
	}

	bool DXGraphicsShader::CreatePixelShader(const char* filePath, const char* entry) {
		if (!CompileShader(filePath, entry, "ps_5_0", _psBlob)) {
			return false;
		}

		if (FAILED(_context.Device()->CreatePixelShader(_psBlob->GetBufferPointer(), _psBlob->GetBufferSize(), nullptr, _pixelShader.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXGraphicsShader::CompileShader(const char* filePath, const char* entryPoint, const char* target, ComPtr<ID3DBlob>& blob) {
		std::wstring wfilename = std::wstring(filePath, filePath + strlen(filePath));

		ComPtr<ID3DBlob> errorBlob = nullptr;
		if (FAILED(D3DCompileFromFile(
			wfilename.c_str(),
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entryPoint,
			target,
			D3DCOMPILE_DEBUG,
			0,
			blob.GetAddressOf(),
			errorBlob.GetAddressOf()))) 
		{
			if (errorBlob) {
				Log::Error("%s", (char*)errorBlob->GetBufferPointer());
			}
			else {
				Log::Error("D3DCompileFromFile failed");
			}

			return false;
		}

		return true;
	}
}

#endif
