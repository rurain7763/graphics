#include "pch.h"
#include "world.h"

struct DirectionalLightInstanceData {
	vec3 direction;
	vec3 color;
};

struct PointLightInstanceData {
	mat4 modelMatrix;
	vec3 position;
	float radius;
	vec3 color;
	vec3 attenuation; // x = constant, y = linear, z = quadratic
};

Ref<ShaderResourcesLayout> g_lightingStaticSRL;
Ref<ShaderResourcesLayout> g_lightingDynamicSRL;
Ref<VertexInputLayout> g_directLightInstanceInputLayout;
Ref<VertexInputLayout> g_pointLightInstanceInputLayout;
Ref<GraphicsPipeline> g_directionalLightingPipeline;
Ref<GraphicsPipeline> g_pointLightingPipeline;

Ref<ShaderResources> g_lightingStaticSR;
Ref<GraphicsResourcesPool<ShaderResources>> g_lightingDynamicSRPool;
Ref<GraphicsResourcesPool<VertexBuffer>> g_directLightInstanceDataPool;
Ref<GraphicsResourcesPool<VertexBuffer>> g_pointLightInstanceDataPool;

DirectionalLightInstanceData g_directionalLightInstanceData;
std::vector<PointLightInstanceData> g_pointLightInstanceDatas;

void Lighting_Init() {
	// NOTE: Create shader resources layout
	ShaderResourcesLayout::Descriptor lightingStaticSRLDesc;
	lightingStaticSRLDesc.bindings = {
		{ 0, ResourceType::ConstantBuffer, ShaderStage::Vertex | ShaderStage::Pixel, 1 },
	};

	g_lightingStaticSRL = g_graphicsContext->CreateShaderResourcesLayout(lightingStaticSRLDesc);

	ShaderResources::Descriptor lightingStaticSRDesc;
	lightingStaticSRDesc.layout = g_lightingStaticSRL;

	g_lightingStaticSR = g_graphicsContext->CreateShaderResources(lightingStaticSRDesc);
	g_lightingStaticSR->BindConstantBuffer(g_cameraCB, 0);

	ShaderResourcesLayout::Descriptor lightingDynamicSRLDesc;
	lightingDynamicSRLDesc.bindings = {
		{ 0, ResourceType::InputAttachment, ShaderStage::Pixel, 1 },
		{ 1, ResourceType::InputAttachment, ShaderStage::Pixel, 1 },
		{ 2, ResourceType::InputAttachment, ShaderStage::Pixel, 1 },
	};

	g_lightingDynamicSRL = g_graphicsContext->CreateShaderResourcesLayout(lightingDynamicSRLDesc);

	g_lightingDynamicSRPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc = { g_lightingDynamicSRL };
		return context.CreateShaderResources(desc);
	});

	// NOTE: Create buffers
	VertexInputLayout::Descriptor directLightInstanceInputLayoutDesc;
	directLightInstanceInputLayoutDesc.inputElements = {
		{ "LDIRECTION", ElementType::Float, 3 },
		{ "LCOLOR", ElementType::Float, 3 },
	};
	directLightInstanceInputLayoutDesc.vertexInputRate = VertexInputRate::Instance;

	g_directLightInstanceInputLayout = g_graphicsContext->CreateVertexInputLayout(directLightInstanceInputLayoutDesc);

	g_directLightInstanceDataPool = CreateRef<GraphicsResourcesPool<VertexBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		VertexBuffer::Descriptor vbDesc;
		vbDesc.memProperty = MemoryProperty::Dynamic;
		vbDesc.elmSize = sizeof(DirectionalLightInstanceData);
		vbDesc.bufferSize = vbDesc.elmSize * 1; // Only one directional light
		return context.CreateVertexBuffer(vbDesc);
	});

	VertexInputLayout::Descriptor pointLightInstanceInputLayoutDesc;
	pointLightInstanceInputLayoutDesc.inputElements = {
		{ "MODEL_MATRIX", ElementType::Float, 4 },
		{ "MODEL_MATRIX1", ElementType::Float, 4 },
		{ "MODEL_MATRIX2", ElementType::Float, 4 },
		{ "MODEL_MATRIX3", ElementType::Float, 4 },
		{ "LPOSITION", ElementType::Float, 3 },
		{ "LRADIUS", ElementType::Float, 1 },
		{ "LCOLOR", ElementType::Float, 3 },
		{ "LATTENUATION", ElementType::Float, 3 },
	};
	pointLightInstanceInputLayoutDesc.vertexInputRate = VertexInputRate::Instance;

	g_pointLightInstanceInputLayout = g_graphicsContext->CreateVertexInputLayout(pointLightInstanceInputLayoutDesc);

	g_pointLightInstanceDataPool = CreateRef<GraphicsResourcesPool<VertexBuffer>>(*g_graphicsContext, [](GraphicsContext& context) {
		VertexBuffer::Descriptor vbDesc;
		vbDesc.memProperty = MemoryProperty::Dynamic;
		vbDesc.elmSize = sizeof(PointLightInstanceData);
		vbDesc.bufferSize = vbDesc.elmSize * MAX_POINT_LIGHTS;
		return context.CreateVertexBuffer(vbDesc);
	});

	// NOTE: Create point lighting pipeline
	GraphicsShader::Descriptor directionalLightingShaderDesc;
	directionalLightingShaderDesc.vertexShaderFile = "assets/shaders/lighting_directional.vert.spv";
	directionalLightingShaderDesc.vertexShaderEntry = "main";
	directionalLightingShaderDesc.pixelShaderFile = "assets/shaders/lighting_directional.frag.spv";
	directionalLightingShaderDesc.pixelShaderEntry = "main";

	auto directionalLightingShader = g_graphicsContext->CreateGraphicsShader(directionalLightingShaderDesc);

	g_directionalLightingPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_directionalLightingPipeline->SetShader(directionalLightingShader);
	g_directionalLightingPipeline->SetShaderResourcesLayouts({ g_lightingStaticSRL, g_lightingDynamicSRL });
	g_directionalLightingPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout, g_directLightInstanceInputLayout });
	g_directionalLightingPipeline->SetRenderPass(g_sceneRenderPass, 1);
	g_directionalLightingPipeline->EnableBlendMode(0, true);
	g_directionalLightingPipeline->SetBlendMode(0, BlendMode::Additive);
	g_directionalLightingPipeline->EnableDepthTest(false);
	g_directionalLightingPipeline->SetCullMode(CullMode::Back);
	g_directionalLightingPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);

	GraphicsShader::Descriptor pointLightingShaderDesc;
	pointLightingShaderDesc.vertexShaderFile = "assets/shaders/lighting_point.vert.spv";
	pointLightingShaderDesc.vertexShaderEntry = "main";
	pointLightingShaderDesc.pixelShaderFile = "assets/shaders/lighting_point.frag.spv";
	pointLightingShaderDesc.pixelShaderEntry = "main";

	auto pointLightingShader = g_graphicsContext->CreateGraphicsShader(pointLightingShaderDesc);

	g_pointLightingPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_pointLightingPipeline->SetShader(pointLightingShader);
	g_pointLightingPipeline->SetShaderResourcesLayouts({ g_lightingStaticSRL, g_lightingDynamicSRL });
	g_pointLightingPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout, g_pointLightInstanceInputLayout });
	g_pointLightingPipeline->SetRenderPass(g_sceneRenderPass, 1);
	g_pointLightingPipeline->EnableBlendMode(0, true);
	g_pointLightingPipeline->SetBlendMode(0, BlendMode::Additive);
	g_pointLightingPipeline->EnableDepthTest(false);
	g_pointLightingPipeline->SetCullMode(CullMode::Front);
	g_pointLightingPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void Lighting_Cleanup() {
	g_lightingStaticSRL.reset();
	g_lightingDynamicSRL.reset();
	g_directLightInstanceInputLayout.reset();
	g_pointLightInstanceInputLayout.reset();
	g_directionalLightingPipeline.reset();
	g_pointLightingPipeline.reset();

	g_lightingStaticSR.reset();
	g_lightingDynamicSRPool.reset();
	g_directLightInstanceDataPool.reset();
	g_pointLightInstanceDataPool.reset();
}

void Lighting_Update() {
	g_lightingDynamicSRPool->Reset();
	g_directLightInstanceDataPool->Reset();
	g_pointLightInstanceDataPool->Reset();

	g_directionalLightInstanceData.direction = g_directionalLight.direction;
	g_directionalLightInstanceData.color = g_directionalLight.color;

	g_pointLightInstanceDatas.resize(g_pointLights.size());
	for (uint32_t i = 0; i < g_pointLights.size(); i++) {
		const auto& pointLight = g_pointLights[i];

		auto& instanceData = g_pointLightInstanceDatas[i];

		instanceData.position = pointLight.position;
		instanceData.color = pointLight.color;
		instanceData.attenuation = vec3(1.0, pointLight.linear, pointLight.quadratic);
		instanceData.radius = pointLight.GetDistance();
		instanceData.modelMatrix = ModelMatrix(instanceData.position, vec3(0.0f), vec3(instanceData.radius * 2));
	}
}

void Lighting_Render() {
	auto& commandQueue = g_graphicsContext->GetCommandQueue();

	auto gBuffer = GetGBuffer();
	auto dynamicSR = g_lightingDynamicSRPool->Get();

	dynamicSR->BindInputAttachment(gBuffer.position, 0);
	dynamicSR->BindInputAttachment(gBuffer.normal, 1);
	dynamicSR->BindInputAttachment(gBuffer.albedoSpec, 2);

	auto quadMesh = GetMesh("quad");

	auto directLightInstanceVB = g_directLightInstanceDataPool->Get();
	directLightInstanceVB->Update(&g_directionalLightInstanceData, sizeof(DirectionalLightInstanceData));

	commandQueue.SetPipeline(g_directionalLightingPipeline);
	commandQueue.SetShaderResources({ g_lightingStaticSR, dynamicSR });
	commandQueue.SetVertexBuffers({ quadMesh->vertexBuffer, directLightInstanceVB });
	commandQueue.DrawIndexedInstanced(quadMesh->indexBuffer, quadMesh->indexBuffer->IndexCount(), 1);
	
	auto sphereMesh = GetMesh("sphere");

	auto pointLightInstanceVB = g_pointLightInstanceDataPool->Get();
	pointLightInstanceVB->Update(g_pointLightInstanceDatas.data(), sizeof(PointLightInstanceData) * g_pointLightInstanceDatas.size());

	commandQueue.SetPipeline(g_pointLightingPipeline);
	commandQueue.SetShaderResources({ g_lightingStaticSR, dynamicSR });
	commandQueue.SetVertexBuffers({ sphereMesh->vertexBuffer, pointLightInstanceVB });
	commandQueue.DrawIndexedInstanced(sphereMesh->indexBuffer, sphereMesh->indexBuffer->IndexCount(), g_pointLightInstanceDatas.size());
}