#include "pch.h"
#include "world.h"

static Ref<ShaderResourcesLayout> g_staticShaderResourcesLayout;
static Ref<ShaderResourcesLayout> g_dynamicShaderResourcesLayout;
static Ref<ShaderResources> g_staticShaderResources;
static Ref<GraphicsResourcesPool<ShaderResources>> g_dynamicShaderResourcesPool;

static Ref<GraphicsPipeline> g_explodePipeline;
static Ref<GraphicsPipeline> g_viewNormalPipeline;

void Geometry_Init() {
	// NOTE: Create shader resources
	ShaderResourcesLayout::Descriptor staticSRLDesc;
	staticSRLDesc.bindings = {
		{ 0, ResourceType::ConstantBuffer, ShaderStage::Geometry | ShaderStage::Pixel, 1 },
		{ 1, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
		{ 2, ResourceType::ConstantBuffer, ShaderStage::Geometry, 1 },
		{ 4, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
		{ 5, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
		{ 6, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
	};
	g_staticShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(staticSRLDesc);

	ShaderResources::Descriptor staticSRDesc;
	staticSRDesc.layout = g_staticShaderResourcesLayout;

	g_staticShaderResources = g_graphicsContext->CreateShaderResources(staticSRDesc);
	g_staticShaderResources->BindConstantBuffer(g_cameraCB, 0);
	g_staticShaderResources->BindConstantBuffer(g_lightCB, 1);
	g_staticShaderResources->BindConstantBuffer(g_globalCB, 2);
	g_staticShaderResources->BindStructuredBuffer(g_directionalLightSB, 4);
	g_staticShaderResources->BindStructuredBuffer(g_pointLightSB, 5);
	g_staticShaderResources->BindStructuredBuffer(g_spotLightSB, 6);

	ShaderResourcesLayout::Descriptor dynamicSRLDesc;
	dynamicSRLDesc.bindings = {
		{ 0, ResourceType::ConstantBuffer, ShaderStage::Geometry, 1 },
		{ 1, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
		{ 2, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
		{ 3, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
		{ 4, ResourceType::TextureCube, ShaderStage::Pixel, 1 },
	};

	g_dynamicShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(dynamicSRLDesc);

	g_dynamicShaderResourcesPool = CreateRef<GraphicsResourcesPool<ShaderResources>>(*g_graphicsContext, [](GraphicsContext& context) {
		ShaderResources::Descriptor desc;
		desc.layout = g_dynamicShaderResourcesLayout;

		return context.CreateShaderResources(desc);
	});

	// NOTE: Create pipeline
	GraphicsShader::Descriptor explodeShaderDesc;
#if USE_VULKAN
	explodeShaderDesc.vertexShaderFile = "assets/shaders/object_pass_through.vert.spv";
	explodeShaderDesc.vertexShaderEntry = "main";
	explodeShaderDesc.geometryShaderFile = "assets/shaders/explode.geom.spv";
	explodeShaderDesc.geometryShaderEntry = "main";
	explodeShaderDesc.pixelShaderFile = "assets/shaders/shader.frag.spv";
	explodeShaderDesc.pixelShaderEntry = "main";
#elif USE_DX11
	explodeShaderDesc.vertexShaderFile = "assets/shaders/object_pass_through.fx";
	explodeShaderDesc.vertexShaderEntry = "VSMain";
	explodeShaderDesc.geometryShaderFile = "assets/shaders/explode.geom.spv";
	explodeShaderDesc.geometryShaderEntry = "main";
	explodeShaderDesc.pixelShaderFile = "assets/shaders/shader.frag.spv";
	explodeShaderDesc.pixelShaderEntry = "main";
#endif

	auto explodeShader = g_graphicsContext->CreateGraphicsShader(explodeShaderDesc);

	g_explodePipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_explodePipeline->SetShader(explodeShader);
	g_explodePipeline->SetRenderPass(g_sceneRenderPass, 0);
	g_explodePipeline->EnableBlendMode(0, true);
	g_explodePipeline->SetBlendMode(0, BlendMode::Default);
	g_explodePipeline->SetShaderResourcesLayouts({ g_staticShaderResourcesLayout, g_dynamicShaderResourcesLayout });
	g_explodePipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_explodePipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);

	GraphicsShader::Descriptor viewNormalShaderDesc;
	viewNormalShaderDesc.vertexShaderFile = "assets/shaders/object_pass_through.vert.spv";
	viewNormalShaderDesc.vertexShaderEntry = "main";
	viewNormalShaderDesc.geometryShaderFile = "assets/shaders/view_normal.geom.spv";
	viewNormalShaderDesc.geometryShaderEntry = "main";
	viewNormalShaderDesc.pixelShaderFile = "assets/shaders/view_normal.frag.spv";
	viewNormalShaderDesc.pixelShaderEntry = "main";

	auto viewNormalShader = g_graphicsContext->CreateGraphicsShader(viewNormalShaderDesc);

	g_viewNormalPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_viewNormalPipeline->SetShader(viewNormalShader);
	g_viewNormalPipeline->SetRenderPass(g_sceneRenderPass, 0);
	g_viewNormalPipeline->EnableBlendMode(0, true);
	g_viewNormalPipeline->SetBlendMode(0, BlendMode::Default);
	g_viewNormalPipeline->SetShaderResourcesLayouts({ g_staticShaderResourcesLayout, g_dynamicShaderResourcesLayout });
	g_viewNormalPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
	g_viewNormalPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);

	// NOTE: Add test object
	auto& obj = AddObject();
	obj.name = "Explode_Effect";
	obj.position = vec3(0, 0, 5);
	obj.scale = vec3(1.0f);

	auto meshComp = obj.AddComponent<StaticMeshComponent>();
	meshComp->mesh = GetMesh("sphere");
	meshComp->excludeFromRendering = true;
}

void Geometry_Cleanup() {
	g_staticShaderResourcesLayout.reset();
	g_dynamicShaderResourcesLayout.reset();
	g_staticShaderResources.reset();
	g_dynamicShaderResourcesPool.reset();
	g_explodePipeline.reset();
	g_viewNormalPipeline.reset();
}

void Geometry_Render() {
	auto& commandQueue = g_graphicsContext->GetCommandQueue();
	auto& obj = GetObjectWithName("Explode_Effect");
	auto meshComp = obj.GetComponent<StaticMeshComponent>();

	g_dynamicShaderResourcesPool->Reset();

	auto dynamicShaderResources = g_dynamicShaderResourcesPool->Get();
	auto objectConstantsCB = g_objConstantsCBPool->Get();
	auto materialConstantsCB = g_objMaterialCBPool->Get();

	dynamicShaderResources->BindConstantBuffer(objectConstantsCB, 0);
	dynamicShaderResources->BindConstantBuffer(materialConstantsCB, 1);

	ObjectConstants objConstants;
	objConstants.model_matrix = ModelMatrix(obj.position, obj.rotation, obj.scale);
	objConstants.inv_model_matrix = glm::inverse(objConstants.model_matrix);

	objectConstantsCB->Update(&objConstants, sizeof(ObjectConstants));

	for (uint32_t i = 0; i < meshComp->mesh->segments.size(); i++) {
		auto& subMesh = meshComp->mesh->segments[i];
		auto material = meshComp->mesh->materials[i];

		MaterialConstants materialConstants = GetMaterialConstants(material);
		materialConstantsCB->Update(&materialConstants, sizeof(MaterialConstants));

		if (material->diffuseTexture) {
			dynamicShaderResources->BindTexture2D(material->diffuseTexture, 2);
		}
		else {
			dynamicShaderResources->BindTexture2D(GetTexture2D("dummy"), 2);
		}

		if (material->specularTexture) {
			dynamicShaderResources->BindTexture2D(material->specularTexture, 3);
		}
		else {
			dynamicShaderResources->BindTexture2D(GetTexture2D("dummy"), 3);
		}

		dynamicShaderResources->BindTextureCube(GetTextureCube("skybox"), 4);

		commandQueue.SetVertexBuffers({ meshComp->mesh->vertexBuffer });
		commandQueue.SetPipeline(g_explodePipeline);
		commandQueue.SetShaderResources({ g_staticShaderResources, dynamicShaderResources });
		commandQueue.DrawIndexed(meshComp->mesh->indexBuffer, subMesh.indexCount, subMesh.indexOffset, subMesh.vertexOffset);
	}

	for (uint32_t index : g_viewNormalObjects) {
		const auto& object = g_objects[index];

		auto meshComp = object.GetComponent<StaticMeshComponent>();

		auto dynamicShaderResources = g_dynamicShaderResourcesPool->Get();
		auto objectConstantsCB = g_objConstantsCBPool->Get();
		auto materialConstantsCB = g_objMaterialCBPool->Get();

		dynamicShaderResources->BindConstantBuffer(objectConstantsCB, 0);
		dynamicShaderResources->BindConstantBuffer(materialConstantsCB, 1);

		ObjectConstants objConstants;
		objConstants.model_matrix = ModelMatrix(object.position, object.rotation, object.scale);
		objConstants.inv_model_matrix = glm::inverse(objConstants.model_matrix);
		objectConstantsCB->Update(&objConstants, sizeof(ObjectConstants));

		for (uint32_t i = 0; i < meshComp->mesh->segments.size(); i++) {
			auto& subMesh = meshComp->mesh->segments[i];
			auto material = meshComp->mesh->materials[i];

			MaterialConstants materialConstants = GetMaterialConstants(material);
			materialConstantsCB->Update(&materialConstants, sizeof(MaterialConstants));
			if (material->diffuseTexture) {
				dynamicShaderResources->BindTexture2D(material->diffuseTexture, 2);
			}
			else {
				dynamicShaderResources->BindTexture2D(GetTexture2D("dummy"), 2);
			}

			if (material->specularTexture) {
				dynamicShaderResources->BindTexture2D(material->specularTexture, 3);
			}
			else {
				dynamicShaderResources->BindTexture2D(GetTexture2D("dummy"), 3);
			}

			dynamicShaderResources->BindTextureCube(GetTextureCube("skybox"), 4);

			commandQueue.SetVertexBuffers({ meshComp->mesh->vertexBuffer });
			commandQueue.SetPipeline(g_viewNormalPipeline);
			commandQueue.SetShaderResources({ g_staticShaderResources, dynamicShaderResources });
			commandQueue.DrawIndexed(meshComp->mesh->indexBuffer, subMesh.indexCount, subMesh.indexOffset, subMesh.vertexOffset);
		}
	}
}

