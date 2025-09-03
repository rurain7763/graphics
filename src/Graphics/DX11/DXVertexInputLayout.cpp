#include "pch.h"
#include "DXBuffers.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXShaders.h"
#include "Log/Log.h"

namespace flaw {
	DXVertexInputLayout::DXVertexInputLayout(DXContext& context, const Descriptor& desc) 
		: _context(context)
		, _inputClassification(ConvertToDXInputClassification(desc.vertexInputRate))
		, _inputElements(desc.inputElements)
	{
		for (auto& element : _inputElements) {
			int32_t firstNumberPos;
			for (firstNumberPos = element.name.size() - 1; firstNumberPos >= 0; firstNumberPos--) {
				if (isdigit(element.name[firstNumberPos])) {
					break;
				}
			}

			if (firstNumberPos < 0) {
				_semanticIndices.push_back(0);
			}
			else {
				uint32_t semanticIndex = std::stoi(element.name.substr(firstNumberPos));
				_semanticIndices.push_back(semanticIndex);
				element.name = element.name.substr(0, firstNumberPos);
			}
		}
	}

	void DXVertexInputLayout::GetInputElements(uint32_t binding, std::vector<D3D11_INPUT_ELEMENT_DESC>& elements) const {
		uint32_t offset = 0;
		for (uint32_t i = 0; i < _inputElements.size(); i++) {
			const auto& element = _inputElements[i];
			uint32_t semanticIndex = _semanticIndices[i];

			DXGI_FORMAT format = ConvertToDXFormat(element.type, element.count);

			if (format == DXGI_FORMAT_UNKNOWN) {
				LOG_ERROR("Invalid element type or count", static_cast<int>(element.type), element.count);
				return;
			}

			D3D11_INPUT_ELEMENT_DESC dxDesc = {};
			dxDesc.SemanticName = element.name.c_str();
			dxDesc.SemanticIndex = semanticIndex;
			dxDesc.Format = format;
			dxDesc.InputSlot = binding;
			dxDesc.AlignedByteOffset = offset;
			dxDesc.InputSlotClass = _inputClassification;
			dxDesc.InstanceDataStepRate = _inputClassification == D3D11_INPUT_PER_VERTEX_DATA ? 0 : 1;

			elements.push_back(dxDesc);

			offset += GetElementSize(element.type) * element.count;
		}
	}
}

#endif