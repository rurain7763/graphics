#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsShaders.h"

#include <string>
#include <array>

namespace flaw {
	class VkContext;

	class VkShaderResourcesLayout : public ShaderResourcesLayout {
	public:
		VkShaderResourcesLayout(VkContext& context, const Descriptor& descriptor);
		~VkShaderResourcesLayout();

		inline const std::vector<vk::DescriptorSetLayoutBinding>& GetVkDescriptorSetLayoutBindings() const {
			return _bindings;
		}

		inline const vk::DescriptorSetLayout& GetVkDescriptorSetLayout() const {
			return _descriptorSetLayout;
		}

	private:
		VkContext& _context;

		vk::DescriptorSetLayout _descriptorSetLayout;
		std::vector<vk::DescriptorSetLayoutBinding> _bindings;
	};

	class VkShaderResources : public ShaderResources {
	public:
		VkShaderResources(VkContext& context, const Descriptor& descriptor);
		~VkShaderResources();

		void BindTexture2D(const Ref<Texture2D>& texture, uint32_t binding) override;
		void BindTexture2DUA(const Ref<Texture2D>& texture, uint32_t binding) override {}

		void BindTextureCube(const Ref<TextureCube>& texture, uint32_t binding) override;

		void BindConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t binding) override;

		void BindStructuredBuffer(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) override;
		void BindStructuredBufferUA(const Ref<StructuredBuffer>& structuredBuffer, uint32_t binding) override {}

		inline const vk::DescriptorSet& GetVkDescriptorSet() const {
			return _descriptorSet;
		}

	private:
		VkContext& _context;

		vk::DescriptorSet _descriptorSet;

		Ref<VkShaderResourcesLayout> _layout;
	};

	class VkGraphicsShader : public GraphicsShader {
	public:
		struct Stage {
			vk::ShaderModule module;
			vk::ShaderStageFlagBits stage;
			std::string entryPoint;
		};

		VkGraphicsShader(VkContext& context, const Descriptor& descriptor);
		~VkGraphicsShader();

		const std::vector<Stage>& GetShaderStages() const { return _shaderStages; }

	private:
		void CreateShader(const std::string& filePath, const std::string& entryPoint, ShaderStage complileFlag);

	private:
		VkContext& _context;

		std::vector<Stage> _shaderStages;
	};

	class VkComputeShader : public ComputeShader {
	public:
		VkComputeShader(VkContext& context, const Descriptor& descriptor);
		~VkComputeShader();
	
		void GetVkShaderStage(vk::PipelineShaderStageCreateInfo& shaderStage) const;

	private:
		bool CreateShader(const std::string& filePath, const std::string& entryPoint);

	private:
		VkContext& _context;

		vk::ShaderModule _shaderModule;
		std::string _entryPoint;
	};
}

#endif