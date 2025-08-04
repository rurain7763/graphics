#pragma once

#include "Graphics/GraphicsShader.h"

#include <string>

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	class DXContext;

	class DXGraphicsShader : public GraphicsShader {
	public:
		DXGraphicsShader(
			DXContext& graphics, 
			const char* filePath,
			const uint32_t compileFlag = ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel
		);

		~DXGraphicsShader() override = default;

		void CreateInputLayout() override;

		void Bind() override;

	private:
		bool CreateVertexShader(const char* filePath);
		bool CreateHullShader(const char* filePath);
		bool CreateDomainShader(const char* filePath);
		bool CreateGeometryShader(const char* filePath);
		bool CreatePixelShader(const char* filePath);

		bool CompileShader(
			const char* filePath,
			const char* entryPoint,
			const char* target,
			ComPtr<ID3DBlob>& blob
		);

		DXGI_FORMAT GetFormat(InputElement::ElementType type, uint32_t count);

	private:
		DXContext& _graphics;

		ComPtr<ID3DBlob> _vsBlob;
		ComPtr<ID3D11VertexShader> _vertexShader;

		ComPtr<ID3DBlob> _hsBlob;
		ComPtr<ID3D11HullShader> _hullShader;

		ComPtr<ID3DBlob> _dsBlob;
		ComPtr<ID3D11DomainShader> _domainShader;

		ComPtr<ID3DBlob> _gsBlob;
		ComPtr<ID3D11GeometryShader> _geometryShader;

		ComPtr<ID3DBlob> _psBlob;
		ComPtr<ID3D11PixelShader> _pixelShader;

		ComPtr<ID3D11InputLayout> _inputLayout;
	};
}
