#pragma once

#include "Core.h"
#include "GraphicsType.h"

#include <vector>

namespace flaw {
	class Texture2D;
	class TextureCube;
	class ConstantBuffer;
	class StructuredBuffer;

	class ShaderResourcesLayout {
	public:
		struct ResourceBinding {
			uint32_t binding;
			ResourceType resourceType;
			ShaderStages shaderStages;
			uint32_t count;
		};

		struct Descriptor {
			std::vector<ResourceBinding> bindings;
		};

		virtual ~ShaderResourcesLayout() = default;
	};

	class ShaderResources {
	public:
		struct Descriptor {
			Ref<ShaderResourcesLayout> layout;
		};

		virtual ~ShaderResources() = default;

		virtual void BindTexture2D(const Ref<Texture2D>& texture, uint32_t binding) = 0;
		virtual void BindTexture2DUA(const Ref<Texture2D>& texture, uint32_t binding) = 0;

		virtual void BindTextureCube(const Ref<TextureCube>& texture, uint32_t binding) = 0;

		virtual void BindConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t binding) = 0;

		virtual void BindStructuredBuffer(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) = 0;
		virtual void BindStructuredBufferUA(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) = 0;
	};

	class GraphicsShader {
	public:
		struct Descriptor {
			std::string vertexShaderFile;
			std::string vertexShaderEntry = "VSMain";

			std::string hullShaderFile;
			std::string hullShaderEntry = "HSMain";
			
			std::string domainShaderFile;
			std::string domainShaderEntry = "DSMain";

			std::string geometryShaderFile;
			std::string geometryShaderEntry = "GSMain";

			std::string pixelShaderFile;
			std::string pixelShaderEntry = "PSMain";
		};

		GraphicsShader() = default;
		virtual ~GraphicsShader() = default;
	};

	class ComputeShader {
	public:
		struct Descriptor {
			std::string file;
			std::string entry = "CSMain";
		};

		ComputeShader() = default;
		virtual ~ComputeShader() = default;
	};
}


