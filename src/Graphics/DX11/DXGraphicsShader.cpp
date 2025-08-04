#include "pch.h"
#include "DXGraphicsShader.h"

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXGraphicsShader::DXGraphicsShader(DXContext& graphics, const char* filePath, const uint32_t compileFlag)
		:_graphics(graphics)
	{
		bool success = true;

		if ((compileFlag & ShaderCompileFlag::Vertex) && !CreateVertexShader(filePath)) {
			success = false;
		}

		if ((compileFlag & ShaderCompileFlag::Hull) && !CreateHullShader(filePath)) {
			success = false;
		}

		if ((compileFlag & ShaderCompileFlag::Domain) && !CreateDomainShader(filePath)) {
			success = false;
		}

		if ((compileFlag & ShaderCompileFlag::Geometry) && !CreateGeometryShader(filePath)) {
			success = false;
		}

		if ((compileFlag & ShaderCompileFlag::Pixel) && !CreatePixelShader(filePath)) {
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

	void DXGraphicsShader::CreateInputLayout() {
		if (_inputLayout) {
			_inputLayout.Reset();
		}

		if (_inputElements.empty()) {
			Log::Error("InputLayout is empty");
			return;
		}

		std::vector<D3D11_INPUT_ELEMENT_DESC> descs;

		for (const auto& attribute : _inputElements) {
			DXGI_FORMAT format = GetFormat(attribute.type, attribute.count);

			if (format == DXGI_FORMAT_UNKNOWN) {
				Log::Error("Unknown format");
				return;
			}

			D3D11_INPUT_ELEMENT_DESC desc = {};
			desc.SemanticName = attribute.name;
			desc.SemanticIndex = 0;
			desc.Format = format;
			desc.InputSlot = 0;
			desc.AlignedByteOffset = attribute.offset;
			desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			desc.InstanceDataStepRate = 0;

			descs.push_back(std::move(desc));
		}

		if (FAILED(_graphics.Device()->CreateInputLayout(
			descs.data(),
			static_cast<uint32_t>(descs.size()),
			_vsBlob->GetBufferPointer(),
			_vsBlob->GetBufferSize(),
			_inputLayout.GetAddressOf()))) 
		{
			Log::Error("CreateInputLayout failed");
			return;
		}
	}

	DXGI_FORMAT DXGraphicsShader::GetFormat(InputElement::ElementType type, uint32_t count) {
		switch (type) {
			case InputElement::ElementType::Float:
				switch (count) {
					case 1: return DXGI_FORMAT_R32_FLOAT;
					case 2: return DXGI_FORMAT_R32G32_FLOAT;
					case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
					case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				}
				break;
			case InputElement::ElementType::Uint32:
				switch (count) {
					case 1: return DXGI_FORMAT_R32_UINT;
					case 2: return DXGI_FORMAT_R32G32_UINT;
					case 3: return DXGI_FORMAT_R32G32B32_UINT;
					case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
				}
				break;
			case InputElement::ElementType::Int:
				switch (count) {
					case 1: return DXGI_FORMAT_R32_SINT;
					case 2: return DXGI_FORMAT_R32G32_SINT;
					case 3: return DXGI_FORMAT_R32G32B32_SINT;
					case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
				}
				break;
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	void DXGraphicsShader::Bind() {
		_graphics.DeviceContext()->VSSetShader(_vertexShader.Get(), nullptr, 0);
		_graphics.DeviceContext()->HSSetShader(_hullShader.Get(), nullptr, 0);
		_graphics.DeviceContext()->DSSetShader(_domainShader.Get(), nullptr, 0);
		_graphics.DeviceContext()->GSSetShader(_geometryShader.Get(), nullptr, 0);
		_graphics.DeviceContext()->PSSetShader(_pixelShader.Get(), nullptr, 0);

		_graphics.DeviceContext()->IASetInputLayout(_inputLayout.Get());
	}

	bool DXGraphicsShader::CreateVertexShader(const char* filePath) {
		if (!CompileShader(filePath, "VSMain", "vs_5_0", _vsBlob)) {
			return false;
		}

		if (FAILED(_graphics.Device()->CreateVertexShader(_vsBlob->GetBufferPointer(), _vsBlob->GetBufferSize(), nullptr, _vertexShader.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXGraphicsShader::CreateHullShader(const char* filePath) {
		if (!CompileShader(filePath, "HSMain", "hs_5_0", _hsBlob)) {
			return false;
		}

		if (FAILED(_graphics.Device()->CreateHullShader(_hsBlob->GetBufferPointer(), _hsBlob->GetBufferSize(), nullptr, _hullShader.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXGraphicsShader::CreateDomainShader(const char* filePath) {
		if (!CompileShader(filePath, "DSMain", "ds_5_0", _dsBlob)) {
			return false;
		}
		if (FAILED(_graphics.Device()->CreateDomainShader(_dsBlob->GetBufferPointer(), _dsBlob->GetBufferSize(), nullptr, _domainShader.GetAddressOf()))) {
			return false;
		}
		return true;
	}

	bool DXGraphicsShader::CreateGeometryShader(const char* filePath) {
		if (!CompileShader(filePath, "GSMain", "gs_5_0", _gsBlob)) {
			return false;
		}
		if (FAILED(_graphics.Device()->CreateGeometryShader(_gsBlob->GetBufferPointer(), _gsBlob->GetBufferSize(), nullptr, _geometryShader.GetAddressOf()))) {
			return false;
		}
		return true;
	}

	bool DXGraphicsShader::CreatePixelShader(const char* filePath) {
		if (!CompileShader(filePath, "PSMain", "ps_5_0", _psBlob)) {
			return false;
		}

		if (FAILED(_graphics.Device()->CreatePixelShader(_psBlob->GetBufferPointer(), _psBlob->GetBufferSize(), nullptr, _pixelShader.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXGraphicsShader::CompileShader(
		const char* filePath,
		const char* entryPoint,
		const char* target,
		ComPtr<ID3DBlob>& blob
	) {
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
