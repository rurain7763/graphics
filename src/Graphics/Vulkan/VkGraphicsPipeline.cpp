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
        _depthStencilInfo.stencilTestEnable = VK_FALSE;
        _depthStencilInfo.front = vk::StencilOpState{};
        _depthStencilInfo.back = vk::StencilOpState{};

        _dynamicStateInfo.dynamicStateCount = 0;
        _dynamicStateInfo.pDynamicStates = _dynamicStates.data();

        SetPrimitiveTopology(PrimitiveTopology::TriangleList);
        SetDepthTest(DepthTest::Less, true);
        SetCullMode(CullMode::Front);
        SetFillMode(FillMode::Solid);
        SetRenderPassLayout(_context.GetMainRenderPassLayout());
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

    void VkGraphicsPipeline::SetDepthTest(DepthTest depthTest, bool depthWrite) {
        vk::CompareOp vkDepthTest = ConvertToVkDepthTest(depthTest);

        if (_depthStencilInfo.depthCompareOp == vkDepthTest && _depthStencilInfo.depthWriteEnable == depthWrite) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _depthStencilInfo.depthTestEnable = vkDepthTest != vk::CompareOp::eNever;
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

    void VkGraphicsPipeline::AddShaderResourcesLayout(const Ref<ShaderResourcesLayout>& shaderResourceLayout) {
        auto vkShaderResourceLayout = std::static_pointer_cast<VkShaderResourcesLayout>(shaderResourceLayout);
        FASSERT(vkShaderResourceLayout, "Invalid shader resource layout type for Vulkan pipeline");

        _needRecreatePipeline = true;

        _shaderResourceLayouts.push_back(vkShaderResourceLayout);
        _descriptorSetLayouts.push_back(vkShaderResourceLayout->GetVkDescriptorSetLayout());

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
        vkShader->GetVkShaderStages(_shaderStages);
    }

    void VkGraphicsPipeline::SetVertexInputLayout(const Ref<GraphicsVertexInputLayout>& vertexInputLayout) {
        if (_vertexInputLayout == vertexInputLayout) {
            return; // No change needed
        }

        auto vkVertexInputLayout = std::static_pointer_cast<VkVertexInputLayout>(vertexInputLayout);
        FASSERT(vkVertexInputLayout, "Invalid vertex input layout type for Vulkan pipeline");

        _needRecreatePipeline = true;

        auto& bindingDescription = vkVertexInputLayout->GetVkVertexInputBindingDescription();
        auto& attributeDescriptions = vkVertexInputLayout->GetVkVertexInputAttributeDescriptions();

        _vertexInputLayout = vkVertexInputLayout;
        _vertexInputState.vertexBindingDescriptionCount = 1;
        _vertexInputState.pVertexBindingDescriptions = &bindingDescription;
        _vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        _vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
    }

    void VkGraphicsPipeline::SetRenderPassLayout(const Ref<GraphicsRenderPassLayout>& renderPassLayout) {
        if (_renderPassLayout == renderPassLayout) {
            return; // No change needed
        }

        auto vkRenderPassLayout = std::static_pointer_cast<VkRenderPassLayout>(renderPassLayout);
        FASSERT(vkRenderPassLayout, "Invalid render pass type for Vulkan pipeline");

        _needRecreatePipeline = true;

        GraphicsRenderPass::Descriptor renderPassDesc;
        renderPassDesc.layout = vkRenderPassLayout;
        renderPassDesc.colorAttachmentOperations.resize(vkRenderPassLayout->GetColorAttachmentCount());
        for (uint32_t i = 0; i < renderPassDesc.colorAttachmentOperations.size(); ++i) {
            auto& op = renderPassDesc.colorAttachmentOperations[i];
            op.initialLayout = TextureLayout::Undefined;
            op.finalLayout = TextureLayout::Color;
            op.loadOp = AttachmentLoadOp::Clear;
            op.storeOp = AttachmentStoreOp::Store;
        }

        renderPassDesc.depthStencilAttachmentOperation = std::nullopt;
        if (vkRenderPassLayout->HasDepthStencilAttachment()) {
            renderPassDesc.depthStencilAttachmentOperation = {
                TextureLayout::Undefined,
                TextureLayout::DepthStencil,
                AttachmentLoadOp::Clear,
                AttachmentStoreOp::Store,
                AttachmentLoadOp::DontCare,
                AttachmentStoreOp::DontCare
            };
        }

        _renderPass = CreateRef<VkRenderPass>(_context, renderPassDesc);

        _colorBlendAttachments.clear();
        for (uint32_t i = 0; i < vkRenderPassLayout->GetColorAttachmentCount(); ++i) {
            const auto& attachment = vkRenderPassLayout->GetColorAttachment(i);

            vk::PipelineColorBlendAttachmentState attachmentState;
            attachmentState.blendEnable = attachment.blendMode != BlendMode::Disabled;
            attachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            switch (attachment.blendMode) {
                case BlendMode::Default:
                    attachmentState.srcColorBlendFactor = vk::BlendFactor::eOne;
                    attachmentState.dstColorBlendFactor = vk::BlendFactor::eZero;
                    attachmentState.colorBlendOp = vk::BlendOp::eAdd;
                    attachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
                    attachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
                    attachmentState.alphaBlendOp = vk::BlendOp::eAdd;
                    break;
                case BlendMode::Alpha:
                    attachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
                    attachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
                    attachmentState.colorBlendOp = vk::BlendOp::eAdd;
                    attachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
                    attachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
                    attachmentState.alphaBlendOp = vk::BlendOp::eAdd;
                    break;
                case BlendMode::Additive:
                    attachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
                    attachmentState.dstColorBlendFactor = vk::BlendFactor::eOne;
                    attachmentState.colorBlendOp = vk::BlendOp::eAdd;
                    attachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
                    attachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
                    attachmentState.alphaBlendOp = vk::BlendOp::eAdd;
                    break;
                case BlendMode::Disabled:
                default:
                    // No blending
                    break;
            }

            _colorBlendAttachments.push_back(attachmentState);
        }

        _colorBlendStateInfo.attachmentCount = _colorBlendAttachments.size();
        _colorBlendStateInfo.pAttachments = _colorBlendAttachments.data();
    }

    void VkGraphicsPipeline::Bind() {
        // TODO: Implement binding logic
    }

    void VkGraphicsPipeline::AddPushConstantRange(const PushConstantRange& pushConstant) {
        _needRecreatePipeline = true;

        if (_pushConstantRanges.size() == 0) {
            _pushConstantOffset = 0;
        }

        vk::PushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = ConvertToVkShaderStages(pushConstant.shaderStages);
        pushConstantRange.offset = _pushConstantOffset;
        pushConstantRange.size = pushConstant.size;

        _pushConstantRanges.push_back(pushConstantRange);

        _pushConstantOffset += pushConstant.size;

        _pipelineLayout = nullptr;
    }

    void VkGraphicsPipeline::SetBehaviorStates(uint32_t flags) {
        if (_behaviorFlags == flags) {
            return; // No change needed
        }

        _needRecreatePipeline = true;

        _behaviorFlags = flags;
        _dynamicStates.clear();

        if (flags & BehaviorFlag::AutoResizeViewport) {
            _dynamicStates.push_back(vk::DynamicState::eViewport);
        }
        if (flags & BehaviorFlag::AutoResizeScissor) {
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
            pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(_pushConstantRanges.size());
            pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();

            auto pipelineLayoutWrapper = _context.GetVkDevice().createPipelineLayout(pipelineLayoutInfo, nullptr);
            if (pipelineLayoutWrapper.result != vk::Result::eSuccess) {
                Log::Fatal("Failed to create Vulkan pipeline layout: %s", vk::to_string(pipelineLayoutWrapper.result).c_str());
                return;
            }

            _pipelineLayout = pipelineLayoutWrapper.value;
        }

        vk::GraphicsPipelineCreateInfo pipelineInfo;
        pipelineInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
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
            Log::Error("Failed to create Vulkan graphics pipeline: %s", vk::to_string(pipelineWrapper.result).c_str());
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