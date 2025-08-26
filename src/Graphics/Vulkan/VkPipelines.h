#pragma once

#include "Core.h"

#ifdef SUPPORT_VULKAN

#include "VkCore.h"
#include "Graphics/GraphicsPipelines.h"

namespace flaw {
	class VkContext;
    class VkVertexInputLayout;
    class VkShaderResourcesLayout;
    class VkRenderPassLayout;
    class VkRenderPass;
    class VkComputeShader;

    struct VkPushConstantRange {
        uint32_t shaderStages;
        uint32_t size;
    };

	class VkGraphicsPipeline : public GraphicsPipeline {
	public:
		VkGraphicsPipeline(VkContext& context);
        ~VkGraphicsPipeline();

        void SetVertexInputLayouts(const std::vector<Ref<VertexInputLayout>>& vertexInputLayouts) override;

        void SetPrimitiveTopology(PrimitiveTopology primitiveTopology) override;
		void SetViewport(float x, float y, float width, float height) override;
        void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) override;

        void EnableDepthTest(bool enable) override;
        void SetDepthTest(CompareOp depthCompareOp, bool depthWrite = true) override;
		void SetCullMode(CullMode cullMode) override;
		void SetFillMode(FillMode fillMode) override;

        void EnableStencilTest(bool enable) override;
        void SetStencilTest(const StencilOperation& frontFace, const StencilOperation& backFace) override;

        void SetShaderResourcesLayouts(const std::vector<Ref<ShaderResourcesLayout>>& shaderResourceLayouts) override;
        void SetShader(const Ref<GraphicsShader>& shader) override;

        void SetRenderPassLayout(const Ref<RenderPassLayout>& renderPassLayout) override;
		void EnableBlendMode(uint32_t attachmentIndex, bool enable) override;
		void SetBlendMode(uint32_t attachmentIndex, BlendMode blendMode) override;
		void SetAlphaToCoverage(bool enable) override;

        void SetBehaviorStates(uint32_t behaviors) override;
        uint32_t GetBehaviorStates() const override;

        void SetPushConstantRanges(const std::vector<VkPushConstantRange>& pushConstants);

        vk::Pipeline GetNativeVkGraphicsPipeline();
        inline vk::PipelineLayout GetVkPipelineLayout() const { return _pipelineLayout; }
        inline const std::vector<vk::PushConstantRange>& GetVkPushConstantRanges() const { return _pushConstantRanges; }
        inline const std::vector<vk::DescriptorSetLayout>& GetVkDescriptorSetLayouts() const { return _descriptorSetLayouts; }
        inline const vk::Viewport& GetVkViewport() const { return _viewport; }
        inline const vk::Rect2D& GetVkScissor() const { return _scissor; }

    private:
        void CreatePipeline();
        void DestroyPipeline();

    private:
        VkContext& _context;

		bool _needRecreatePipeline;

        vk::Pipeline _pipeline;

		std::vector<Ref<VkVertexInputLayout>> _vertexInputLayouts;
		std::vector<vk::VertexInputBindingDescription> _bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> _attributeDescriptions;
        vk::PipelineVertexInputStateCreateInfo _vertexInputState;

        vk::PipelineInputAssemblyStateCreateInfo _inputAssemblyInfo;

        vk::Viewport _viewport;
        vk::Rect2D _scissor;
        vk::PipelineViewportStateCreateInfo _viewportStateInfo;

        vk::PipelineRasterizationStateCreateInfo _rasterizationInfo;

        std::vector<Ref<VkShaderResourcesLayout>> _shaderResourceLayouts;
        std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;

        Ref<GraphicsShader> _shader;
        std::vector<vk::PipelineShaderStageCreateInfo> _shaderStages;

        vk::PipelineMultisampleStateCreateInfo _multisampleInfo;

        std::vector<vk::PipelineColorBlendAttachmentState> _colorBlendAttachments;
        vk::PipelineColorBlendStateCreateInfo _colorBlendStateInfo;

        vk::PipelineDepthStencilStateCreateInfo _depthStencilInfo;

        std::vector<vk::DynamicState> _dynamicStates;
        vk::PipelineDynamicStateCreateInfo _dynamicStateInfo;

        vk::PipelineLayout _pipelineLayout;
        std::vector<vk::PushConstantRange> _pushConstantRanges;

        Ref<VkRenderPassLayout> _renderPassLayout;
        Ref<VkRenderPass> _renderPass;

        uint32_t _behaviorFlags;
	};

    class VkComputePipeline : public ComputePipeline {
    public:
        VkComputePipeline(VkContext& context);
        ~VkComputePipeline();

        void SetShader(const Ref<ComputeShader>& shader) override;
        void AddShaderResourcesLayout(const Ref<ShaderResourcesLayout>& shaderResourceLayout) override;
        
        void AddPushConstantRange(const VkPushConstantRange& pushConstant);

        vk::Pipeline GetNativeVkComputePipeline();

    private:
        void CreatePipeline();
        void DestroyPipeline();

    private:
        VkContext& _context;

        bool _needRecreatePipeline;

        vk::Pipeline _pipeline;
        vk::PipelineLayout _pipelineLayout;

        Ref<VkComputeShader> _shader;

        std::vector<Ref<VkShaderResourcesLayout>> _shaderResourceLayouts;
        std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;

        uint32_t _pushConstantOffset;
        std::vector<vk::PushConstantRange> _pushConstantRanges;
    };
}

#endif