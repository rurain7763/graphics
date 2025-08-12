#include "pch.h"
#include "DXShaders.h"

#ifdef SUPPORT_DX11

#include "DXContext.h"
#include "DXTextures.h"
#include "DXBuffers.h"
#include "Log/Log.h"

namespace flaw {
	DXShaderResourcesLayout::DXShaderResourcesLayout(DXContext& context, const Descriptor& descriptor)
		: _context(context)
	{
		for (size_t i = 0; i < descriptor.bindings.size(); ++i) {
			const auto& binding = descriptor.bindings[i];

			switch (binding.resourceType) {
			case ResourceType::Texture2D:
			case ResourceType::TextureCube:
			case ResourceType::StructuredBuffer:
				_tRegistryBindings[binding.binding] = binding;
				break;
			case ResourceType::ConstantBuffer:
				_cRegistryBindings[binding.binding] = binding;
				break;
			};
		}
	}

	DXShaderResources::DXShaderResources(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _layout(std::static_pointer_cast<DXShaderResourcesLayout>(descriptor.layout))
	{
		if (!_layout) {
			LOG_ERROR("Invalid ShaderResourcesLayout provided");
			return;
		}
	}

	void DXShaderResources::BindTexture2D(const Ref<Texture2D>& texture, uint32_t binding) {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);
		FASSERT(dxTexture, "Invalid Texture2D provided");

		const auto& tRegistryBindings = _layout->GetTRegistryBindings();

		auto resBindingIt = tRegistryBindings.find(binding);
		if (resBindingIt == tRegistryBindings.end()) {
			LOG_ERROR("Binding %d not found in shader resources", binding);
			return;
		}

		auto& resBinding = resBindingIt->second;

		if ((texture->GetShaderStages() & resBinding.shaderStages) != resBinding.shaderStages) {
			LOG_ERROR("Texture2D binding %d does not match shader stages", binding);
			return;
		}

		_tRegistryResources[binding] = dxTexture->GetNativeSRV();
	}

	void DXShaderResources::BindTexture2DUA(const Ref<Texture2D>& texture, uint32_t binding) {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);
		FASSERT(dxTexture, "Invalid Texture2D provided");

		const auto& uRegistryBindings = _layout->GetURegistryBindings();

		auto resBindingIt = uRegistryBindings.find(binding);
		if (resBindingIt == uRegistryBindings.end()) {
			LOG_ERROR("Binding %d not found in shader resources", binding);
			return;
		}

		auto& resBinding = resBindingIt->second;
		if ((texture->GetShaderStages() & resBinding.shaderStages) != resBinding.shaderStages) {
			LOG_ERROR("Texture2D UA binding %d does not match shader stages", binding);
			return;
		}

		_uRegistryResources[binding] = dxTexture->GetNativeUAV();
	}

	void DXShaderResources::BindTextureCube(const Ref<TextureCube>& texture, uint32_t binding) {
		auto dxTexture = std::static_pointer_cast<DXTextureCube>(texture);
		FASSERT(dxTexture, "Invalid TextureCube provided");

		const auto& tRegistryBindings = _layout->GetTRegistryBindings();

		auto resBindingIt = tRegistryBindings.find(binding);
		if (resBindingIt == tRegistryBindings.end()) {
			LOG_ERROR("Binding %d not found in shader resources", binding);
			return;
		}

		auto& resBinding = resBindingIt->second;
		if ((texture->GetShaderStages() & resBinding.shaderStages) != resBinding.shaderStages) {
			LOG_ERROR("TextureCube binding %d does not match shader stages", binding);
			return;
		}

		_tRegistryResources[binding] = dxTexture->GetNativeSRV();
	}

	void DXShaderResources::BindConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t binding) {
		auto dxBuffer = std::static_pointer_cast<DXConstantBuffer>(constantBuffer);
		FASSERT(dxBuffer, "Invalid ConstantBuffer provided");

		const auto& cRegistryBindings = _layout->GetCRegistryBindings();

		if (cRegistryBindings.find(binding) == cRegistryBindings.end()) {
			LOG_ERROR("Binding %d not found in shader resources", binding);
			return;
		}

		_cRegistryResources[binding] = dxBuffer->GetNativeBuffer();
	}

	void DXShaderResources::BindStructuredBuffer(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) {
		auto dxBuffer = std::static_pointer_cast<DXStructuredBuffer>(structuredBuffer);
		FASSERT(dxBuffer, "Invalid StructuredBuffer provided");

		const auto& tRegistryBindings = _layout->GetTRegistryBindings();

		if (tRegistryBindings.find(binding) == tRegistryBindings.end()) {
			LOG_ERROR("Binding %d not found in shader resources", binding);
			return;
		}

		_tRegistryResources[binding] = dxBuffer->GetNativeSRV();
	}

	void DXShaderResources::BindStructuredBufferUA(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) {
		auto dxBuffer = std::static_pointer_cast<DXStructuredBuffer>(structuredBuffer);
		FASSERT(dxBuffer, "Invalid StructuredBuffer provided");

		const auto& uRegistryBindings = _layout->GetURegistryBindings();

		if (uRegistryBindings.find(binding) == uRegistryBindings.end()) {
			LOG_ERROR("Binding %d not found in shader resources", binding);
			return;
		}

		_uRegistryResources[binding] = dxBuffer->GetNativeUAV();
	}
}

#endif