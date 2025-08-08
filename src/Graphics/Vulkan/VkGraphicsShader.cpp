#include "pch.h"
#include "VkShaders.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Platform/FileSystem.h"
#include "Log/Log.h"

namespace flaw {
    VkGraphicsShader::VkGraphicsShader(VkContext& context, const Descriptor& descriptor) 
        : _context(context)
    {
        _compileFlags = 0;
        if (!descriptor.vertexShaderFile.empty()) {
            _compileFlags |= ShaderStage::Vertex;
            CreateShader(descriptor.vertexShaderFile.c_str(), descriptor.vertexShaderEntry, ShaderStage::Vertex);
        }

        if (!descriptor.hullShaderFile.empty()) {
            _compileFlags |= ShaderStage::Hull;
            CreateShader(descriptor.hullShaderFile.c_str(), descriptor.hullShaderEntry, ShaderStage::Hull);
        }

        if (!descriptor.domainShaderFile.empty()) {
            _compileFlags |= ShaderStage::Domain;
            CreateShader(descriptor.domainShaderFile.c_str(), descriptor.domainShaderEntry, ShaderStage::Domain);
        }

        if (!descriptor.geometryShaderFile.empty()) {
            _compileFlags |= ShaderStage::Geometry;
            CreateShader(descriptor.geometryShaderFile.c_str(), descriptor.geometryShaderEntry, ShaderStage::Geometry);
        }

        if (!descriptor.pixelShaderFile.empty()) {
            _compileFlags |= ShaderStage::Pixel;
            CreateShader(descriptor.pixelShaderFile.c_str(), descriptor.pixelShaderEntry, ShaderStage::Pixel);
        }
    }

    void VkGraphicsShader::CreateShader(const std::string& filePath, const std::string& entryPoint, ShaderStage compileFlag) {
        std::vector<int8_t> sourceCode;
        if (!FileSystem::ReadFile(filePath.c_str(), sourceCode)) {
            Log::Error("Failed to read shader file: %s", filePath.c_str());
            return;
        }

        vk::ShaderModuleCreateInfo createInfo;
        createInfo.codeSize = sourceCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(sourceCode.data());

        auto moduleWrapper = _context.GetVkDevice().createShaderModule(createInfo, nullptr);
        if (moduleWrapper.result != vk::Result::eSuccess) {
            Log::Error("Failed to create shader module: %s", filePath.c_str());
            return;
        }

        ShaderEntry entry;
        entry.module = moduleWrapper.value;
        entry.entryPoint = entryPoint;
        entry.stage = ConvertToVkShaderStage(compileFlag);

        _shaderEntries.push_back(entry);
    }

    VkGraphicsShader::~VkGraphicsShader() {
        for (const auto& shaderEntry : _shaderEntries) {
            if (!shaderEntry.module) {
                continue;
            }

            _context.AddDelayedDeletionTasks([&context = _context, shaderModule = shaderEntry.module]() {
                context.GetVkDevice().destroyShaderModule(shaderModule, nullptr);
            });
        }
    }

    void VkGraphicsShader::Bind() {
        // Implementation for binding the shader
    }

    void VkGraphicsShader::GetVkShaderStages(std::vector<vk::PipelineShaderStageCreateInfo>& shaderStages) const {
        shaderStages.resize(_shaderEntries.size());
        std::transform(_shaderEntries.begin(), _shaderEntries.end(), shaderStages.begin(),
            [](const ShaderEntry& entry) {
                vk::PipelineShaderStageCreateInfo stageInfo;
                stageInfo.stage = entry.stage;
                stageInfo.module = entry.module;
                stageInfo.pName = entry.entryPoint.c_str();
                return stageInfo;
            }
        );
    }
}

#endif