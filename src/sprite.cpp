#include "pch.h"
#include "sprite.h"
#include "world.h"

#include <map>

static std::vector<std::vector<Ref<ConstantBuffer>>> g_objectConstantsCBsPerFrame;
static uint32_t g_objectConstantsCBUsed = 0;
static Ref<ShaderResourcesLayout> g_staticShaderResourcesLayout;
static Ref<ShaderResources> g_staticShaderResources;
static Ref<ShaderResourcesLayout> g_dynamicShaderResourcesLayout;
static std::vector<std::vector<Ref<ShaderResources>>> g_dynamicShaderResourcesPerFrame;
static uint32_t g_dynamicShaderResourcesUsed = 0;
static Ref<GraphicsPipeline> g_spritePipeline;

#if USE_VULKAN
const uint32_t cameraConstantsCBBinding = 0;
const uint32_t objectConstantsCBBinding = 0;
const uint32_t diffuseTextureBinding = 1;
#elif USE_DX11
const uint32_t cameraConstantsCBBinding = 0;
const uint32_t objectConstantsCBBinding = 1;
const uint32_t diffuseTextureBinding = 0;
#endif

void Sprite_Init() {
    // NOTE: Create buffers
	g_objectConstantsCBsPerFrame.resize(g_graphicsContext->GetMainFramebuffersCount());

    // NOTE: Create shader resources
	ShaderResourcesLayout::Descriptor staticSRLDesc;
	staticSRLDesc.bindings = {
		{ cameraConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
	};

	g_staticShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(staticSRLDesc);

	ShaderResources::Descriptor staticSRDesc;
	staticSRDesc.layout = g_staticShaderResourcesLayout;

	g_staticShaderResources = g_graphicsContext->CreateShaderResources(staticSRDesc);
	g_staticShaderResources->BindConstantBuffer(g_cameraCB, cameraConstantsCBBinding);

	ShaderResourcesLayout::Descriptor dynamicSRLDesc;
	dynamicSRLDesc.bindings = {
		{ objectConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
        { diffuseTextureBinding, ResourceType::Texture2D, ShaderStage::Pixel, 1 }
    };

	g_dynamicShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(dynamicSRLDesc);

	g_dynamicShaderResourcesPerFrame.resize(g_graphicsContext->GetMainFramebuffersCount());

    // NOTE: Create pipelines
	GraphicsShader::Descriptor spriteShaderDesc;
#if USE_VULKAN
	spriteShaderDesc.vertexShaderFile = "assets/shaders/object.vert.spv";
	spriteShaderDesc.vertexShaderEntry = "main";
    spriteShaderDesc.pixelShaderFile = "assets/shaders/sprite.frag.spv";
    spriteShaderDesc.pixelShaderEntry = "main";
#else USE_DX11
	spriteShaderDesc.vertexShaderFile = "assets/shaders/object.fx";
	spriteShaderDesc.vertexShaderEntry = "VSMain";
    spriteShaderDesc.pixelShaderFile = "assets/shaders/sprite.fx";
    spriteShaderDesc.pixelShaderEntry = "PSMain";
#endif

	auto spriteShader = g_graphicsContext->CreateGraphicsShader(spriteShaderDesc);

	g_spritePipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_spritePipeline->SetShader(spriteShader);
    g_spritePipeline->SetCullMode(CullMode::None);
    g_spritePipeline->EnableBlendMode(0, true);
    g_spritePipeline->SetBlendMode(0, BlendMode::Alpha);
	g_spritePipeline->SetShaderResourcesLayouts({ g_staticShaderResourcesLayout, g_dynamicShaderResourcesLayout });
	g_spritePipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_spritePipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void Sprite_Cleanup() {
    g_objectConstantsCBsPerFrame.clear();
    g_staticShaderResourcesLayout.reset();
    g_staticShaderResources.reset();
    g_dynamicShaderResourcesLayout.reset();
    g_dynamicShaderResourcesPerFrame.clear();
    g_spritePipeline.reset();
}

static Ref<ShaderResources> GetDynamicShaderResources(uint32_t frameIndex) {
	if (g_dynamicShaderResourcesUsed >= g_dynamicShaderResourcesPerFrame[frameIndex].size()) {
		ShaderResources::Descriptor shaderResourcesDesc;
		shaderResourcesDesc.layout = g_dynamicShaderResourcesLayout;

		auto shaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
		g_dynamicShaderResourcesPerFrame[frameIndex].push_back(shaderResources);
	}

	return g_dynamicShaderResourcesPerFrame[frameIndex][g_dynamicShaderResourcesUsed++];
}

static Ref<ConstantBuffer> GetObjectConstantsCB(uint32_t frameIndex) {
	if (g_objectConstantsCBUsed >= g_objectConstantsCBsPerFrame[frameIndex].size()) {
		ConstantBuffer::Descriptor constantBufferDesc;
		constantBufferDesc.memProperty = MemoryProperty::Dynamic;
		constantBufferDesc.bufferSize = sizeof(ObjectConstants);
		constantBufferDesc.initialData = nullptr;

		auto constantBuffer = g_graphicsContext->CreateConstantBuffer(constantBufferDesc);
		g_objectConstantsCBsPerFrame[frameIndex].push_back(constantBuffer);
	}

	return g_objectConstantsCBsPerFrame[frameIndex][g_objectConstantsCBUsed++];
}

void Sprite_Render() {
    auto& commandQueue = g_graphicsContext->GetCommandQueue();
    uint32_t frameIndex = commandQueue.GetCurrentFrameIndex();

    g_objectConstantsCBUsed = 0;
    g_dynamicShaderResourcesUsed = 0;

    auto quadMesh = g_meshes["quad"];

    std::map<float, Object*> sorted;
    vec3 cameraPos = g_camera->GetPosition();
    for (auto& object : g_objects) {
        if (!object.HasComponent<SpriteComponent>()) {
            continue;
        }

        float distance = glm::distance(cameraPos, object.position);
        sorted[distance] = &object;
    }

    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
        auto spriteComponent = it->second->GetComponent<SpriteComponent>();

        auto objectConstantsCB = GetObjectConstantsCB(frameIndex);
        auto dynamicShaderResources = GetDynamicShaderResources(frameIndex);
    
        ObjectConstants objConstants;
        objConstants.model_matrix = ModelMatrix(it->second->position, it->second->rotation, it->second->scale);
        objConstants.inv_model_matrix = glm::inverse(objConstants.model_matrix);
        
        objectConstantsCB->Update(&objConstants, sizeof(ObjectConstants));

        dynamicShaderResources->BindConstantBuffer(objectConstantsCB, objectConstantsCBBinding);
        
        if (!spriteComponent->texture) {
            spriteComponent->texture = g_textures["dummy"];
        }
        else {
            dynamicShaderResources->BindTexture2D(spriteComponent->texture, diffuseTextureBinding);
        }

        commandQueue.SetPipeline(g_spritePipeline);
        commandQueue.SetVertexBuffers({ quadMesh->vertexBuffer });
        commandQueue.SetShaderResources({ g_staticShaderResources, dynamicShaderResources });
        commandQueue.DrawIndexed(quadMesh->indexBuffer, quadMesh->indexBuffer->IndexCount());
    }
}