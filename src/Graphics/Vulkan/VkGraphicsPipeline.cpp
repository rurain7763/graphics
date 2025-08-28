#include "pch.h"
#include "VkPipelines.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Log/Log.h"
#include "VkShaders.h"
#include "VkBuffers.h"
#include "VkRenderPass.h"

namespace flaw {
    VkGraphicsPipeline::VkGraphicsPipeline(VkContext& context)
        : _context(context) 
        , _needRecreatePipeline(true)
        , _behaviorFlags(0)
    {
        _inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        _vertexInputState.vertexBindingDescriptionCount = 0;
        _vertexInputState.vertexAttributeDescriptionCount = 0;

        _viewportStateInfo.viewportCount = 1;
        _viewportStateInfo.pViewports = &_viewport;
        _viewportStateInfo.scissorCount = 1;
        _viewportStateInfo.pScissors = &_scissor;

        _rasterizationInfo.depthClampEnable = VK_FALSE;
        _rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        _rasterizationInfo.lineWidth = 1.0f;
        _rasterizationInfo.frontFace = vk::FrontFace::eClockwise;
        _rasterizationInfo.depthBiasEnable = VK_FALSE;
        _rasterizationInfo.depthBiasConstantFactor = 0.0f;
        _rasterizationInfo.depthBiasClamp = 0.0f;
        _rasterizationInfo.depthBiasSlopeFactor = 0.0f;

        _multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
        _multisampleInfo.sampleShadingEnable = VK_FALSE;
        _multisampleInfo.minSampleShading = 1.0f;
        _multisampleInfo.pSampleMask = nullptr;
        _multisampleInfo.alphaToCoverageEnable = VK_FALSE;
        _multisampleInfo.alphaToOneEnable = VK_FALSE;

        _colorBlendStateInfo.logicOpEnable = VK_FALSE;
        _colorBlendStateInfo.logicOp = vk::LogicOp::eCopy;
        _colorBlendStateInfo.blendConstants[0] = 0.0f;
        _colorBlendStateInfo.blendConstants[1] = 0.0f;
        _colorBlendStateInfo.blendConstants[2] = 0.0f;
        _colorBlendStateInfo.blendConstants[3] = 0.0f;

        _depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        _depthStencilInfo.minDepthBounds = 0.0f;
        _depthStencilInfo.maxDepthBounds = 1.0f;

        _dynamicStateInfo.dynamicStateCount = 0;
        _dynamicStateInfo.pDynamicStates = _dynamicStates.data();

        SetPrimitiveTopology(PrimitiveTopology::TriangleList);
        EnableDepthTest(true);
        SetDepthTest(CompareOp::Less, true);
        EnableStencilTest(false);
        SetCullMode(CullMode::Back);
        SetFillMode(FillMode::Solid);
        SetRenderPassLayout(_context.GetMainRenderPassLayout());
		EnableBlendMode(0, true);
		SetBlendMode(0, BlendMode::Default);
		SetAlphaToCoverage(false);
    }

    VkGraphicsPipeline::~VkGraphicsPipeline() {
        _context.AddDelayedDeletionTasks([&context = _context, pipeline = _pipeline, pipelineLayout = _pipelineLayout]() {
            if (pipeline) {
                context.GetVkDevice().destroyPipeline(pipeline, nullptr);
            }

            if (pipelineLayout) {
                context.GetVkDevice().destroyPipelineLayout(pipelineLayout, nullptr);
            }
        });
    }

    void VkGraphicsPipeline::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
        vk::PrimitiveTopology vkTopology = ConvertToVkPrimitiveTopology(primitiveTopology);

        if (_inputAssemblyInfo.topology == vkTopology) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _inputAssemblyInfo.topology = vkTopology;
    }

    void VkGraphicsPipeline::SetViewport(float x, float y, float width, float height) {
        if (_viewport.x == x && _viewport.y == y && _viewport.width == width && _viewport.height == height) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _viewport.x = x;
        _viewport.y = y;
        _viewport.width = width;
        _viewport.height = height;
        _viewport.minDepth = 0.0f;
        _viewport.maxDepth = 1.0f;
    }

    void VkGraphicsPipeline::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) {
        if (_scissor.offset.x == x && _scissor.offset.y == y && _scissor.extent.width == width && _scissor.extent.height == height) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _scissor.offset.x = x;
        _scissor.offset.y = y;
        _scissor.extent.width = width;
        _scissor.extent.height = height;
    }

    void VkGraphicsPipeline::EnableDepthTest(bool enable) {
        if (_depthStencilInfo.depthTestEnable == enable) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _depthStencilInfo.depthTestEnable = enable;
    }

    void VkGraphicsPipeline::SetDepthTest(CompareOp depthTest, bool depthWrite) {
        if (!_depthStencilInfo.depthTestEnable) {
			LOG_ERROR("Depth test is not enabled. Cannot set depth test parameters.");
			return;
        }

        vk::CompareOp vkDepthTest = ConvertToVkCompareOp(depthTest);

        if (_depthStencilInfo.depthCompareOp == vkDepthTest && _depthStencilInfo.depthWriteEnable == depthWrite) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _depthStencilInfo.depthCompareOp = vkDepthTest;
        _depthStencilInfo.depthWriteEnable = depthWrite;
    }

    void VkGraphicsPipeline::SetCullMode(CullMode cullMode) {
        vk::CullModeFlags vkCullMode = ConvertToVkCullMode(cullMode);
        if (_rasterizationInfo.cullMode == vkCullMode) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _rasterizationInfo.cullMode = vkCullMode;
    }

    void VkGraphicsPipeline::SetFillMode(FillMode fillMode) {
        vk::PolygonMode vkFillMode = ConvertToVkFillMode(fillMode);
        if (_rasterizationInfo.polygonMode == vkFillMode) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _rasterizationInfo.polygonMode = vkFillMode;
    }

    void VkGraphicsPipeline::EnableStencilTest(bool enable) {
        if (_depthStencilInfo.stencilTestEnable == enable) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _depthStencilInfo.stencilTestEnable = enable;
    }

    void VkGraphicsPipeline::SetStencilTest(const StencilOperation& frontFace, const StencilOperation& backFace) {
        if (!_depthStencilInfo.stencilTestEnable) {
			LOG_ERROR("Stencil test is not enabled. Cannot set stencil operations.");
			return;
        }
        
        _needRecreatePipeline = true;

        vk::StencilOpState frontStencilOpState;
        frontStencilOpState.failOp = ConvertToVkStencilOp(frontFace.failOp);
        frontStencilOpState.passOp = ConvertToVkStencilOp(frontFace.passOp);
        frontStencilOpState.depthFailOp = ConvertToVkStencilOp(frontFace.depthFailOp);
        frontStencilOpState.compareOp = ConvertToVkCompareOp(frontFace.compareOp);
        frontStencilOpState.reference = frontFace.reference;
        frontStencilOpState.writeMask = frontFace.writeMask;
		frontStencilOpState.compareMask = frontFace.compareMask;

        vk::StencilOpState backStencilOpState;
        backStencilOpState.failOp = ConvertToVkStencilOp(backFace.failOp);
        backStencilOpState.passOp = ConvertToVkStencilOp(backFace.passOp);
        backStencilOpState.depthFailOp = ConvertToVkStencilOp(backFace.depthFailOp);
        backStencilOpState.compareOp = ConvertToVkCompareOp(backFace.compareOp);
        backStencilOpState.reference = backFace.reference;
        backStencilOpState.writeMask = backFace.writeMask;
		backStencilOpState.compareMask = backFace.compareMask;

        _depthStencilInfo.front = frontStencilOpState;
        _depthStencilInfo.back = backStencilOpState;
    }

    void VkGraphicsPipeline::SetShaderResourcesLayouts(const std::vector<Ref<ShaderResourcesLayout>>& shaderResourceLayouts) {
        _needRecreatePipeline = true;

		_shaderResourceLayouts.clear();
		_descriptorSetLayouts.clear();
        for (uint32_t i = 0; i < shaderResourceLayouts.size(); i++) {
            auto vkShaderResourceLayout = std::static_pointer_cast<VkShaderResourcesLayout>(shaderResourceLayouts[i]);
            FASSERT(vkShaderResourceLayout, "Invalid shader resource layout type for Vulkan pipeline");
            
            _shaderResourceLayouts.push_back(vkShaderResourceLayout);
            _descriptorSetLayouts.push_back(vkShaderResourceLayout->GetVkDescriptorSetLayout());
        }

        _pipelineLayout = nullptr;
    }

    void VkGraphicsPipeline::SetShader(const Ref<GraphicsShader>& shader) {
        if (_shader == shader) {
            return; // No change needed
        }

        auto vkShader = std::static_pointer_cast<VkGraphicsShader>(shader);
        FASSERT(vkShader, "Invalid shader type for Vulkan pipeline");

        _needRecreatePipeline = true;

        _shader = vkShader;

        const auto& shaderStages = vkShader->GetShaderStages();
        
        _shaderStages.resize(shaderStages.size());
        for (size_t i = 0; i < shaderStages.size(); ++i) {
            const auto& stage = shaderStages[i];
            
            auto& vkShaderStage = _shaderStages[i];
            vkShaderStage.stage = stage.stage;
            vkShaderStage.module = stage.module;
            vkShaderStage.pName = stage.entryPoint.c_str();
        }
    }

    void VkGraphicsPipeline::SetVertexInputLayouts(const std::vector<Ref<VertexInputLayout>>& vertexInputLayouts) {
        _needRecreatePipeline = true;

		_vertexInputLayouts.clear();
        _bindingDescriptions.clear();
        _attributeDescriptions.clear();
		for (uint32_t i = 0; i < vertexInputLayouts.size(); ++i) {
			auto vkVertexInputLayout = std::static_pointer_cast<VkVertexInputLayout>(vertexInputLayouts[i]);
			FASSERT(vkVertexInputLayout, "Invalid vertex input layout type for Vulkan pipeline");

            const auto& vkBindingDescription = vkVertexInputLayout->GetVkVertexInputBindingDescription();
            const auto& vkAttributeDescriptions = vkVertexInputLayout->GetVkVertexInputAttributeDescriptions();

			_vertexInputLayouts.push_back(vkVertexInputLayout);
			_bindingDescriptions.push_back(vkBindingDescription);
			_attributeDescriptions.insert(_attributeDescriptions.end(), vkAttributeDescriptions.begin(), vkAttributeDescriptions.end());
		}

		_vertexInputState.vertexBindingDescriptionCount = _bindingDescriptions.size();
		_vertexInputState.pVertexBindingDescriptions = _bindingDescriptions.data();
        _vertexInputState.vertexAttributeDescriptionCount = _attributeDescriptions.size();
        _vertexInputState.pVertexAttributeDescriptions = _attributeDescriptions.data();
    }

    void VkGraphicsPipeline::SetRenderPassLayout(const Ref<RenderPassLayout>& renderPassLayout) {
        if (_renderPassLayout == renderPassLayout) {
            return; // No change needed
        }

        auto vkRenderPassLayout = std::static_pointer_cast<VkRenderPassLayout>(renderPassLayout);
        FASSERT(vkRenderPassLayout, "Invalid render pass type for Vulkan pipeline");

        _needRecreatePipeline = true;

        RenderPass::Descriptor renderPassDesc;
        renderPassDesc.layout = vkRenderPassLayout;
        renderPassDesc.colorAttachmentOps.resize(vkRenderPassLayout->GetColorAttachmentCount());
        for (uint32_t i = 0; i < renderPassDesc.colorAttachmentOps.size(); ++i) {
            auto& op = renderPassDesc.colorAttachmentOps[i];
            op.initialLayout = TextureLayout::Undefined;
            op.finalLayout = TextureLayout::ColorAttachment;
            op.loadOp = AttachmentLoadOp::Clear;
            op.storeOp = AttachmentStoreOp::Store;
        }

        if (vkRenderPassLayout->HasDepthStencilAttachment()) {
            renderPassDesc.depthStencilAttachmentOp = {
                TextureLayout::Undefined,
                TextureLayout::DepthStencilAttachment,
                AttachmentLoadOp::Clear,
                AttachmentStoreOp::Store,
                AttachmentLoadOp::DontCare,
                AttachmentStoreOp::DontCare
            };
        }

        if (vkRenderPassLayout->HasResolveAttachment()) {
            renderPassDesc.resolveAttachmentOp = {
                TextureLayout::Undefined,
                TextureLayout::ColorAttachment,
                AttachmentLoadOp::Clear,
                AttachmentStoreOp::Store
            };
        }

        _renderPass = CreateRef<VkRenderPass>(_context, renderPassDesc);

		_colorBlendAttachments.resize(vkRenderPassLayout->GetColorAttachmentCount());
        _colorBlendStateInfo.attachmentCount = _colorBlendAttachments.size();
        _colorBlendStateInfo.pAttachments = _colorBlendAttachments.data();

        _multisampleInfo.rasterizationSamples = ConvertToVkSampleCount(vkRenderPassLayout->GetSampleCount());
    }

	void VkGraphicsPipeline::EnableBlendMode(uint32_t attachmentIndex, bool enable) {
		if (attachmentIndex >= _colorBlendAttachments.size()) {
			LOG_ERROR("Attachment index out of bounds for blend mode enabling");
			return;
		}

		auto& blendAttachment = _colorBlendAttachments[attachmentIndex];
		if (blendAttachment.blendEnable == enable) {
			return; // No change needed
		}

		_needRecreatePipeline = true;

		blendAttachment.blendEnable = enable;
        if (enable) {
		    blendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        }
        else {
            blendAttachment.colorWriteMask = {};
        }
	}

	void VkGraphicsPipeline::SetBlendMode(uint32_t attachmentIndex, BlendMode blendMode) {
		if (attachmentIndex >= _colorBlendAttachments.size()) {
			LOG_ERROR("Attachment index out of bounds for blend mode setting");
			return;
		}

		auto& blendAttachment = _colorBlendAttachments[attachmentIndex];

		if (!blendAttachment.blendEnable) {
			LOG_ERROR("Blend mode cannot be set when blending is disabled for attachment index %u", attachmentIndex);
			return;
		}

		_needRecreatePipeline = true;

        switch (blendMode) {
        case BlendMode::Default:
            blendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
            blendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
            blendAttachment.colorBlendOp = vk::BlendOp::eAdd;
            blendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            blendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            blendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
            break;
        case BlendMode::Alpha:
            blendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
            blendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
            blendAttachment.colorBlendOp = vk::BlendOp::eAdd;
            blendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            blendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            blendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
            break;
        case BlendMode::Additive:
            blendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
            blendAttachment.dstColorBlendFactor = vk::BlendFactor::eOne;
            blendAttachment.colorBlendOp = vk::BlendOp::eAdd;
            blendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            blendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            blendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
            break;
        }
	}

	void VkGraphicsPipeline::SetAlphaToCoverage(bool enable) {
		if (_multisampleInfo.alphaToCoverageEnable == enable) {
			return; // No change needed
		}

		_needRecreatePipeline = true;

        _multisampleInfo.alphaToCoverageEnable = enable;
	}

    void VkGraphicsPipeline::SetPushConstantRanges(const std::vector<VkPushConstantRange>& pushConstants) {
        _needRecreatePipeline = true;

        uint32_t offset = 0;
        _pushConstantRanges.clear();
        for (uint32_t i = 0; i < pushConstants.size(); i++) {
			const auto& pushConstant = pushConstants[i];

            vk::PushConstantRange pushConstantRange;
            pushConstantRange.stageFlags = ConvertToVkShaderStages(pushConstant.shaderStages);
            pushConstantRange.offset = offset;
            pushConstantRange.size = pushConstant.size;

            _pushConstantRanges.push_back(pushConstantRange);

            offset += pushConstant.size;
        }

        _pipelineLayout = nullptr;
    }

    void VkGraphicsPipeline::SetBehaviorStates(uint32_t flags) {
        if (_behaviorFlags == flags) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _behaviorFlags = flags;
        _dynamicStates.clear();

        if (flags & Behavior::AutoResizeViewport) {
            _dynamicStates.push_back(vk::DynamicState::eViewport);
        }
        if (flags & Behavior::AutoResizeScissor) {
            _dynamicStates.push_back(vk::DynamicState::eScissor);
        }

        _dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(_dynamicStates.size());
        _dynamicStateInfo.pDynamicStates = _dynamicStates.data();
    }

    uint32_t VkGraphicsPipeline::GetBehaviorStates() const {
        return _behaviorFlags;
    }

    vk::Pipeline VkGraphicsPipeline::GetNativeVkGraphicsPipeline() {
        if (_needRecreatePipeline) {
            DestroyPipeline();
            CreatePipeline();
            _needRecreatePipeline = false;
        }

        return _pipeline;
    }    

    void VkGraphicsPipeline::CreatePipeline() {
        if (!_pipelineLayout) {
            vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
            pipelineLayoutInfo.setLayoutCount = _descriptorSetLayouts.size();
            pipelineLayoutInfo.pSetLayouts = _descriptorSetLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = _pushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();

            auto pipelineLayoutWrapper = _context.GetVkDevice().createPipelineLayout(pipelineLayoutInfo, nullptr);
            if (pipelineLayoutWrapper.result != vk::Result::eSuccess) {
                LOG_ERROR("Failed to create Vulkan pipeline layout: %s", vk::to_string(pipelineLayoutWrapper.result).c_str());
                return;
            }

            _pipelineLayout = pipelineLayoutWrapper.value;
        }

        vk::GraphicsPipelineCreateInfo pipelineInfo;
        pipelineInfo.stageCount = _shaderStages.size();
        pipelineInfo.pStages = _shaderStages.data();
        pipelineInfo.pVertexInputState = &_vertexInputState;
        pipelineInfo.pInputAssemblyState = &_inputAssemblyInfo;
        pipelineInfo.pViewportState = &_viewportStateInfo;
        pipelineInfo.pRasterizationState = &_rasterizationInfo;
        pipelineInfo.pMultisampleState = &_multisampleInfo;
        pipelineInfo.pColorBlendState = &_colorBlendStateInfo;
        pipelineInfo.pDepthStencilState = &_depthStencilInfo;
        pipelineInfo.pDynamicState = &_dynamicStateInfo;
        pipelineInfo.layout = _pipelineLayout;
        pipelineInfo.renderPass = _renderPass->GetNativeVkRenderPass(); 
        pipelineInfo.subpass = 0; // Assuming single subpass for now
        pipelineInfo.basePipelineHandle = nullptr;

        auto pipelineWrapper = _context.GetVkDevice().createGraphicsPipeline(nullptr, pipelineInfo, nullptr);
        if (pipelineWrapper.result != vk::Result::eSuccess) {
            LOG_ERROR("Failed to create Vulkan graphics pipeline: %s", vk::to_string(pipelineWrapper.result).c_str());
            return;
        }

        _pipeline = pipelineWrapper.value;
    }

    void VkGraphicsPipeline::DestroyPipeline() {
        if (_pipeline) {
            _context.GetVkDevice().destroyPipeline(_pipeline, nullptr);
            _pipeline = nullptr;
        }
    }
}

#endif