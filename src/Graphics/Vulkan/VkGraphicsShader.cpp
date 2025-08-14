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
        _shaderStageFlags = 0;
        if (!descriptor.vertexShaderFile.empty()) {
            _shaderStageFlags |= ShaderStage::Vertex;
            CreateShader(descriptor.vertexShaderFile.c_str(), descriptor.vertexShaderEntry, ShaderStage::Vertex);
        }

        if (!descriptor.hullShaderFile.empty()) {
            _shaderStageFlags |= ShaderStage::Hull;
            CreateShader(descriptor.hullShaderFile.c_str(), descriptor.hullShaderEntry, ShaderStage::Hull);
        }

        if (!descriptor.domainShaderFile.empty()) {
            _shaderStageFlags |= ShaderStage::Domain;
            CreateShader(descriptor.domainShaderFile.c_str(), descriptor.domainShaderEntry, ShaderStage::Domain);
        }

        if (!descriptor.geometryShaderFile.empty()) {
            _shaderStageFlags |= ShaderStage::Geometry;
            CreateShader(descriptor.geometryShaderFile.c_str(), descriptor.geometryShaderEntry, ShaderStage::Geometry);
        }

        if (!descriptor.pixelShaderFile.empty()) {
            _shaderStageFlags |= ShaderStage::Pixel;
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

        Stage stage;
        stage.module = moduleWrapper.value;
        stage.entryPoint = entryPoint;
        stage.stage = ConvertToVkShaderStage(compileFlag);

        _shaderStages.push_back(stage);
    }

    VkGraphicsShader::~VkGraphicsShader() {
        for (const auto& shaderStage : _shaderStages) {
            if (!shaderStage.module) {
                continue;
            }

            _context.AddDelayedDeletionTasks([&context = _context, shaderModule = shaderStage.module]() {
                context.GetVkDevice().destroyShaderModule(shaderModule, nullptr);
            });
        }
    }
}

#endif