#include "pch.h"
#include "VkPipelines.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "VkShaders.h"
#include "Log/Log.h"

namespace flaw {
    VkComputePipeline::VkComputePipeline(VkContext& context)
        : _context(context)
        , _needRecreatePipeline(true)
    {
    }

    VkComputePipeline::~VkComputePipeline() {
        _context.AddDelayedDeletionTasks([&context = _context, pipeline = _pipeline, pipelineLayout = _pipelineLayout]() {
            if (pipeline) {
                context.GetVkDevice().destroyPipeline(pipeline);
            }

            if (pipelineLayout) {
                context.GetVkDevice().destroyPipelineLayout(pipelineLayout);
            }
        });
    }

    void VkComputePipeline::SetShader(const Ref<ComputeShader>& shader) {
        if (_shader == shader) {
            return;
        }
        
        auto vkShader = std::static_pointer_cast<VkComputeShader>(shader);
        FASSERT(vkShader, "Invalid shader type for Vulkan compute pipeline");
        
        _needRecreatePipeline = true;

        _shader = vkShader;
    }

    void VkComputePipeline::AddShaderResourcesLayout(const Ref<ShaderResourcesLayout>& shaderResourceLayout) {
        auto vkShaderResourceLayout = std::static_pointer_cast<VkShaderResourcesLayout>(shaderResourceLayout);
        FASSERT(vkShaderResourceLayout, "Invalid shader resource layout type for Vulkan compute pipeline");

        _needRecreatePipeline = true;

        _shaderResourceLayouts.push_back(vkShaderResourceLayout);
        _descriptorSetLayouts.push_back(vkShaderResourceLayout->GetVkDescriptorSetLayout());

        _pipelineLayout = nullptr;
    }

    void VkComputePipeline::AddPushConstantRange(const VkPushConstantRange& pushConstant) {
        _needRecreatePipeline = true;

        vk::PushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = ConvertToVkShaderStages(pushConstant.shaderStages);
        pushConstantRange.offset = _pushConstantOffset;
        pushConstantRange.size = pushConstant.size;

        _pushConstantRanges.push_back(pushConstantRange);

        _pushConstantOffset += pushConstant.size;

        _pipelineLayout = nullptr;
    }

    vk::Pipeline VkComputePipeline::GetNativeVkComputePipeline() {
        if (_needRecreatePipeline) {
            DestroyPipeline();
            CreatePipeline();
            _needRecreatePipeline = false;
        }

        return _pipeline;
    }

    void VkComputePipeline::CreatePipeline() {
        if (!_pipelineLayout) {
            vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
            pipelineLayoutInfo.setLayoutCount = _descriptorSetLayouts.size();
            pipelineLayoutInfo.pSetLayouts = _descriptorSetLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = _pushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = _pushConstantRanges.data();

            auto pipelineLayoutWrapper = _context.GetVkDevice().createPipelineLayout(pipelineLayoutInfo, nullptr);
            if (pipelineLayoutWrapper.result != vk::Result::eSuccess) {
                Log::Fatal("Failed to create Vulkan compute pipeline layout: %s", vk::to_string(pipelineLayoutWrapper.result).c_str());
                return;
            }

            _pipelineLayout = pipelineLayoutWrapper.value;
        }

        vk::ComputePipelineCreateInfo pipelineInfo;
        pipelineInfo.layout = _pipelineLayout;
        _shader->GetVkShaderStage(pipelineInfo.stage);

        auto result = _context.GetVkDevice().createComputePipeline({}, pipelineInfo);
        if (result.result != vk::Result::eSuccess) {
            Log::Fatal("Failed to create Vulkan compute pipeline: %s", vk::to_string(result.result).c_str());
            return;
        }

        _pipeline = result.value;
    }

    void VkComputePipeline::DestroyPipeline() {
        if (_pipeline) {
            _context.AddDelayedDeletionTasks([&context = _context, pipeline = _pipeline]() {
                context.GetVkDevice().destroyPipeline(pipeline);
            });
        }

        _pipeline = VK_NULL_HANDLE;
    }
}

#endif