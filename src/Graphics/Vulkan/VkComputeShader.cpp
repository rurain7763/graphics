#include "pch.h"
#include "VkShaders.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Platform/FileSystem.h"
#include "Log/Log.h"

namespace flaw {
    VkComputeShader::VkComputeShader(VkContext& context, const Descriptor& descriptor)
        : _context(context)
    {
        if (!CreateShader(descriptor.file, descriptor.entry)) {
            return;
        }
    }

    VkComputeShader::~VkComputeShader() {
        _context.AddDelayedDeletionTasks([&context = _context, shaderModule = _shaderModule]() {
            context.GetVkDevice().destroyShaderModule(shaderModule, nullptr);
        });
    }

    void VkComputeShader::GetVkShaderStage(vk::PipelineShaderStageCreateInfo& shaderStage) const {
        shaderStage.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStage.module = _shaderModule;
        shaderStage.pName = _entryPoint.c_str();
    }

    bool VkComputeShader::CreateShader(const std::string& filePath, const std::string& entryPoint) {
        std::vector<int8_t> sourceCode;
        if (!FileSystem::ReadFile(filePath.c_str(), sourceCode)) {
            Log::Error("Failed to read compute shader file: %s", filePath.c_str());
            return false;
        }
        
        vk::ShaderModuleCreateInfo shaderCreateInfo;
        shaderCreateInfo.codeSize = static_cast<uint32_t>(sourceCode.size());
        shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(sourceCode.data());

        auto moduleWrapper = _context.GetVkDevice().createShaderModule(shaderCreateInfo);
        if (moduleWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to create compute shader module: %s", vk::to_string(moduleWrapper.result).c_str());
            return false;
        }

        _shaderModule = moduleWrapper.value;
        _entryPoint = entryPoint;

        return true;
    }
}

#endif