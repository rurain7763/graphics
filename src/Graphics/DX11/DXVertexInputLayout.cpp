#include "pch.h"
#include "DXBuffers.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXShaders.h"
#include "Log/Log.h"

namespace flaw {
	DXVertexInputLayout::DXVertexInputLayout(DXContext& context, const Descriptor& desc) 
		: _context(context)
	{

		uint32_t stride = 0;
		_inputElements.reserve(desc.inputElements.size());
		for (const auto& element : desc.inputElements) {
			DXGI_FORMAT format = ConvertToDXFormat(element.type, element.count);

			if (format == DXGI_FORMAT_UNKNOWN) {
				LOG_ERROR("Invalid element type or count", static_cast<int>(element.type), element.count);
				return;
			}

			char* name = new char[element.name.size() + 1];
			std::copy(element.name.begin(), element.name.end(), name);
			name[element.name.size()] = '\0';

			D3D11_INPUT_ELEMENT_DESC dxDesc = {};
			dxDesc.SemanticName = name;
			dxDesc.SemanticIndex = 0;
			dxDesc.Format = format;
			dxDesc.InputSlot = desc.binding;
			dxDesc.AlignedByteOffset = stride;
			dxDesc.InputSlotClass = ConvertToDXInputClassification(desc.vertexInputRate);
			dxDesc.InstanceDataStepRate = 0;

			_inputElements.push_back(dxDesc);

			stride += GetElementSize(element.type) * element.count;
		}
	}

	DXVertexInputLayout::~DXVertexInputLayout() {
		for (auto& element : _inputElements) {
			delete[] element.SemanticName;
		}
	}

	ComPtr<ID3D11InputLayout> DXVertexInputLayout::GetDXInputLayout(Ref<GraphicsShader> shader) const {
		auto dxShader = std::static_pointer_cast<DXGraphicsShader>(shader);
		FASSERT(dxShader, "Invalid shader type for DXVertexInputLayout");

		ComPtr<ID3DBlob> vsBlob = dxShader->GetDXVertexShaderBlob();
		if (!vsBlob) {
			LOG_ERROR("Vertex shader blob is null");
			return nullptr;
		}

		ComPtr<ID3D11InputLayout> inputLayout;
		if (FAILED(_context.Device()->CreateInputLayout(_inputElements.data(), _inputElements.size(), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inputLayout.GetAddressOf()))) {
			LOG_ERROR("CreateInputLayout failed");
			return nullptr;
		}

		return inputLayout;
	}
}

#endif