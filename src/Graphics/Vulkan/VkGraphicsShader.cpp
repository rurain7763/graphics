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
            _compileFlags |= ShaderCompileFlag::Vertex;
            CreateShader(descriptor.vertexShaderFile.c_str(), descriptor.vertexShaderEntry, ShaderCompileFlag::Vertex);
        }

        if (!descriptor.hullShaderFile.empty()) {
            _compileFlags |= ShaderCompileFlag::Hull;
            CreateShader(descriptor.hullShaderFile.c_str(), descriptor.hullShaderEntry, ShaderCompileFlag::Hull);
        }

        if (!descriptor.domainShaderFile.empty()) {
            _compileFlags |= ShaderCompileFlag::Domain;
            CreateShader(descriptor.domainShaderFile.c_str(), descriptor.domainShaderEntry, ShaderCompileFlag::Domain);
        }

        if (!descriptor.geometryShaderFile.empty()) {
            _compileFlags |= ShaderCompileFlag::Geometry;
            CreateShader(descriptor.geometryShaderFile.c_str(), descriptor.geometryShaderEntry, ShaderCompileFlag::Geometry);
        }

        if (!descriptor.pixelShaderFile.empty()) {
            _compileFlags |= ShaderCompileFlag::Pixel;
            CreateShader(descriptor.pixelShaderFile.c_str(), descriptor.pixelShaderEntry, ShaderCompileFlag::Pixel);
        }
    }

    void VkGraphicsShader::CreateShader(const std::string& filePath, const std::string& entryPoint, ShaderCompileFlag compileFlag) {
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

            _context.GetVkDevice().destroyShaderModule(shaderEntry.module, nullptr);
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