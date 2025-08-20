#include "pch.h"
#include "world.h"
#include "Platform/PlatformEvents.h"
#include "Event/EventDispatcher.h"
#include "Graphics/Vulkan/VkContext.h"
#include "Graphics/DX11/DXContext.h"
#include "Graphics/GraphicsFunc.h"
#include "Time/Time.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "Model/Model.h"
#include "Log/Log.h"

Ref<PlatformContext> g_context;
EventDispatcher g_eventDispatcher;
Ref<GraphicsContext> g_graphicsContext;
Ref<ConstantBuffer> g_cameraCB;
Ref<ConstantBuffer> g_lightCB;
Ref<ConstantBuffer> g_materialCB;
Ref<StructuredBuffer> g_modelMatricesSB;
Ref<GraphicsPipeline> g_skyboxPipeline;
Ref<GraphicsPipeline> g_objPipeline;
#if USE_VULKAN
Ref<ShaderResourcesLayout> g_skyboxShaderResourcesLayout;
Ref<ShaderResources> g_skyboxShaderResources;
Ref<ShaderResourcesLayout> g_skyboxTexShaderResourcesLayout;
Ref<ShaderResources> g_skyboxTexShaderResources;
Ref<ShaderResourcesLayout> g_objShaderResourcesLayout;
Ref<ShaderResources> g_objShaderResources;
Ref<ShaderResourcesLayout> g_objTexShaderResourcesLayout;
#elif USE_DX11
Ref<ShaderResourcesLayout> g_skyboxShaderResourcesLayout;
Ref<ShaderResources> g_skyboxShaderResources;
Ref<ShaderResourcesLayout> g_objShaderResourcesLayout;
Ref<ShaderResources> g_objShaderResources;
#endif

std::vector<std::vector<Ref<ShaderResources>>> g_objTexShaderResourcesPerFrame;
uint32_t g_objTexShaderResourcesUsed;

std::vector<Object> g_cubeObjects;
std::unordered_map<std::string, Ref<Texture2D>> g_textures;
std::unordered_map<std::string, Ref<TextureCube>> g_textureCubes;
std::unordered_map<std::string, Ref<Mesh>> g_meshes;
std::unordered_map<std::string, Ref<Material>> g_materials;

CameraConstants g_cameraConstants;
LightConstants g_lightConstants;

void InitAssets();

void InitBaseBuffers();

void InitSkyboxShaderResources();
void InitSkyboxGraphicsPipeline();

void InitObjectBuffers();
void InitObjectShaderResources();
void InitObjectGraphicsPipeline();

void World_Init() {
    Log::Initialize();

    const int32_t windowWidth = 800;
    const int32_t windowHeight = 600;

    g_context = CreateRef<PlatformContext>("Flaw Application", windowWidth, windowHeight, g_eventDispatcher);
#if USE_VULKAN
    g_graphicsContext = CreateRef<VkContext>(*g_context, windowWidth, windowHeight);
    MathParams::InvertYAxis = true;
    ModelParams::LeftHanded = true;
#elif USE_DX11
	g_graphicsContext = CreateRef<DXContext>(*g_context, windowWidth, windowHeight);
	MathParams::InvertYAxis = false;
	ModelParams::LeftHanded = true;
#endif

    g_eventDispatcher.Register<WindowResizeEvent>([](const WindowResizeEvent& event) {
        g_graphicsContext->Resize(event.frameBufferWidth, event.frameBufferHeight);
    }, 0);

    g_cameraConstants.view_matrix = ViewMatrix(vec3(0.f, 0.f, -5.f), vec3(0.f));
    g_cameraConstants.projection_matrix = Perspective(glm::radians(45.0f), static_cast<float>(windowWidth) / windowHeight, 0.1f, 100.0f);

#if DIRECT_LIGHTING
	g_lightConstants.direction = Forward;
	g_lightConstants.ambient = glm::vec3(0.2f);
	g_lightConstants.diffuse = glm::vec3(0.8f);
	g_lightConstants.specular = glm::vec3(1.0f);
#elif POINT_LIGHTING
	g_lightConstants.position = glm::vec3(0.0f, 0.0f, 0.0f);
	g_lightConstants.constant_attenuation = 1.0f;
	g_lightConstants.linear_attenuation = 0.09f;
	g_lightConstants.quadratic_attenuation = 0.032f;
    g_lightConstants.ambient = glm::vec3(0.2f);
    g_lightConstants.diffuse = glm::vec3(0.8f);
    g_lightConstants.specular = glm::vec3(1.0f);
#elif SPOT_LIGHTING
	g_lightConstants.position = glm::vec3(0.0f, 0.0f, -2.0f);
	g_lightConstants.direction = Forward;
	g_lightConstants.cutoff_inner_cosine = glm::cos(glm::radians(45.f));
	g_lightConstants.cutoff_outer_cosine = glm::cos(glm::radians(50.0f));
	g_lightConstants.constant_attenuation = 1.0f;
	g_lightConstants.linear_attenuation = 0.09f;
	g_lightConstants.quadratic_attenuation = 0.032f;
	g_lightConstants.ambient = glm::vec3(0.2f);
	g_lightConstants.diffuse = glm::vec3(0.8f);
	g_lightConstants.specular = glm::vec3(1.0f);
#endif

    InitAssets();
    InitBaseBuffers();
    InitSkyboxShaderResources();
    InitSkyboxGraphicsPipeline();
    InitObjectBuffers();
    InitObjectShaderResources();
    InitObjectGraphicsPipeline();
}

void InitAssets() {
    Image hausImg("./assets/textures/haus.jpg", 4);
    Texture2D::Descriptor imageDesc;
    imageDesc.width = hausImg.Width();
    imageDesc.height = hausImg.Height();
    imageDesc.data = hausImg.Data().data();
    imageDesc.memProperty = MemoryProperty::Static;
    imageDesc.texUsages = TextureUsage::ShaderResource;
    imageDesc.format = PixelFormat::RGBA8;
    imageDesc.mipLevels = GetMaxMipLevels(imageDesc.width, imageDesc.height);
    imageDesc.shaderStages = ShaderStage::Pixel;

    g_textures["haus"] = g_graphicsContext->CreateTexture2D(imageDesc);

    Image container2Img("assets/textures/container2.png", 4);
    imageDesc.width = container2Img.Width();
    imageDesc.height = container2Img.Height();
    imageDesc.data = container2Img.Data().data();
    imageDesc.mipLevels = GetMaxMipLevels(imageDesc.width, imageDesc.height);
    imageDesc.shaderStages = ShaderStage::Pixel;

    g_textures["container2"] = g_graphicsContext->CreateTexture2D(imageDesc);

	Image container2SpecularImg("assets/textures/container2_specular.png", 4);
	imageDesc.width = container2SpecularImg.Width();
	imageDesc.height = container2SpecularImg.Height();
	imageDesc.data = container2SpecularImg.Data().data();
	imageDesc.mipLevels = GetMaxMipLevels(imageDesc.width, imageDesc.height);
	imageDesc.shaderStages = ShaderStage::Pixel;

	g_textures["container2_specular"] = g_graphicsContext->CreateTexture2D(imageDesc);

    Ref<Material> defaultMaterial = CreateRef<Material>();
    defaultMaterial->diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    defaultMaterial->diffuseTexture = g_textures["container2"];
    defaultMaterial->specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	defaultMaterial->specularTexture = g_textures["container2_specular"];
    defaultMaterial->shininess = 32.0f;

    g_materials["default"] = defaultMaterial;

    Ref<Mesh> cubeMesh = CreateRef<Mesh>();
    
    std::vector<TexturedVertex> cubeVertices;
    std::vector<uint32_t> cubeIndices;
	GenerateCube(
		[&](const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& binormal) {
			TexturedVertex vertex;
			vertex.position = position;
			vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			vertex.texCoord = texCoord;
			vertex.normal = normal;
			cubeVertices.push_back(vertex);
		},
		cubeIndices
	);

	VertexBuffer::Descriptor vertexBufferDesc;
	vertexBufferDesc.memProperty = MemoryProperty::Static;
	vertexBufferDesc.elmSize = sizeof(TexturedVertex);
	vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * cubeVertices.size();
	vertexBufferDesc.initialData = cubeVertices.data();

	cubeMesh->vertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

	IndexBuffer::Descriptor indexBufferDesc;
	indexBufferDesc.memProperty = MemoryProperty::Static;
	indexBufferDesc.bufferSize = sizeof(uint32_t) * cubeIndices.size();
	indexBufferDesc.initialData = cubeIndices.data();

	cubeMesh->indexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

	SubMesh subMesh;
    subMesh.vertexOffset = 0;
    subMesh.indexOffset = 0;
    subMesh.indexCount = cubeIndices.size();
    subMesh.materialIndex = 0;
	cubeMesh->subMeshes.push_back(subMesh);

    cubeMesh->materials.push_back(defaultMaterial);

	g_meshes["cube"] = cubeMesh;

	Ref<Mesh> sphereMesh = CreateRef<Mesh>();

	Model sphereModel("./assets/models/sphere.obj");

	std::vector<TexturedVertex> sphereVertices;
	for (const auto& vertex : sphereModel.GetVertices()) {
		TexturedVertex texturedVertex;
		texturedVertex.position = vertex.position;
		texturedVertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		texturedVertex.texCoord = vertex.texCoord;
		texturedVertex.normal = vertex.normal;
		sphereVertices.push_back(texturedVertex);
	}

	const std::vector<uint32_t>& sphereIndices = sphereModel.GetIndices();

	vertexBufferDesc.memProperty = MemoryProperty::Static;
	vertexBufferDesc.elmSize = sizeof(TexturedVertex);
	vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * sphereVertices.size();
	vertexBufferDesc.initialData = sphereVertices.data();

	sphereMesh->vertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

	indexBufferDesc.memProperty = MemoryProperty::Static;
	indexBufferDesc.bufferSize = sizeof(uint32_t) * sphereIndices.size();
	indexBufferDesc.initialData = sphereIndices.data();

	sphereMesh->indexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

	for (const auto& mesh : sphereModel.GetMeshs()) {
		SubMesh subMesh;
		subMesh.vertexOffset = mesh.vertexStart;
		subMesh.indexOffset = mesh.indexStart;
		subMesh.indexCount = mesh.indexCount;
        subMesh.materialIndex = 0;
		sphereMesh->subMeshes.push_back(subMesh);
	}

    sphereMesh->materials.push_back(defaultMaterial);

	g_meshes["sphere"] = sphereMesh;

    Ref<Mesh> girlMesh = CreateRef<Mesh>();
    Model girlModel("./assets/models/girl.obj");

    std::vector<TexturedVertex> modelVertices;
    for (const auto& vertex : girlModel.GetVertices()) {
        TexturedVertex texturedVertex;
        texturedVertex.position = vertex.position;
        texturedVertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        texturedVertex.texCoord = vertex.texCoord;
        texturedVertex.normal = vertex.normal;
        modelVertices.push_back(texturedVertex);
    }

    const std::vector<uint32_t>& modelIndices = girlModel.GetIndices();

    vertexBufferDesc.memProperty = MemoryProperty::Static;
    vertexBufferDesc.elmSize = sizeof(TexturedVertex);
    vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * modelVertices.size();
    vertexBufferDesc.initialData = modelVertices.data();

    girlMesh->vertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

    indexBufferDesc.memProperty = MemoryProperty::Static;
    indexBufferDesc.bufferSize = sizeof(uint32_t) * modelIndices.size();
    indexBufferDesc.initialData = modelIndices.data();

    girlMesh->indexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

    for (const auto& mesh : girlModel.GetMeshs()) {
        SubMesh subMesh;
        subMesh.vertexOffset = mesh.vertexStart;
        subMesh.indexOffset = mesh.indexStart;
        subMesh.indexCount = mesh.indexCount;
        subMesh.materialIndex = 0; 
        girlMesh->subMeshes.push_back(subMesh);
    }

    girlMesh->materials.push_back(defaultMaterial);

    g_meshes["girl"] = girlMesh;

    Image left("./assets/textures/sky/sky_left.png", 4);
    Image right("./assets/textures/sky/sky_right.png", 4);
    Image top("./assets/textures/sky/sky_top.png", 4);
    Image bottom("./assets/textures/sky/sky_bottom.png", 4);
    Image front("./assets/textures/sky/sky_front.png", 4);
    Image back("./assets/textures/sky/sky_back.png", 4);

    std::vector<uint8_t> textureData = GenerateTextureCubeData(left, right, top, bottom, front, back);

    TextureCube::Descriptor skyboxDesc = {};
    skyboxDesc.width = left.Width();
    skyboxDesc.height = left.Height();
    skyboxDesc.data = textureData.data();
    skyboxDesc.format = PixelFormat::RGBA8;
    skyboxDesc.memProperty = MemoryProperty::Static;
    skyboxDesc.texUsages = TextureUsage::ShaderResource;
    skyboxDesc.mipLevels = GetMaxMipLevels(skyboxDesc.width, skyboxDesc.height);
	skyboxDesc.shaderStages = ShaderStage::Pixel;

    g_textureCubes["skybox"] = g_graphicsContext->CreateTextureCube(skyboxDesc);
}

void InitBaseBuffers() {
    ConstantBuffer::Descriptor cameraConstantsDesc;
	cameraConstantsDesc.memProperty = MemoryProperty::Dynamic;
	cameraConstantsDesc.bufferSize = sizeof(CameraConstants);
    cameraConstantsDesc.initialData = &g_cameraConstants;
    
    g_cameraCB = g_graphicsContext->CreateConstantBuffer(cameraConstantsDesc);

	ConstantBuffer::Descriptor lightConstantsDesc;
	lightConstantsDesc.memProperty = MemoryProperty::Dynamic;
	lightConstantsDesc.bufferSize = sizeof(LightConstants);
    lightConstantsDesc.initialData = &g_lightConstants;

    g_lightCB = g_graphicsContext->CreateConstantBuffer(lightConstantsDesc);

	ConstantBuffer::Descriptor materialConstantsDesc;
	materialConstantsDesc.memProperty = MemoryProperty::Dynamic;
	materialConstantsDesc.bufferSize = sizeof(MaterialConstants);

	g_materialCB = g_graphicsContext->CreateConstantBuffer(materialConstantsDesc);
}

void InitSkyboxShaderResources() {
#if USE_VULKAN
    ShaderResourcesLayout::Descriptor skyboxResourceLayoutDesc;
    skyboxResourceLayoutDesc.bindings = {
        { 0, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 }
    };

    g_skyboxShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(skyboxResourceLayoutDesc);

    ShaderResources::Descriptor skyboxResourcesDesc;
    skyboxResourcesDesc.layout = g_skyboxShaderResourcesLayout;

    g_skyboxShaderResources = g_graphicsContext->CreateShaderResources(skyboxResourcesDesc);
    g_skyboxShaderResources->BindConstantBuffer(g_cameraCB, 0);

    skyboxResourceLayoutDesc.bindings = {
        { 0, ResourceType::TextureCube, ShaderStage::Pixel, 1 },
    };

    g_skyboxTexShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(skyboxResourceLayoutDesc);

    skyboxResourcesDesc.layout = g_skyboxTexShaderResourcesLayout;

    g_skyboxTexShaderResources = g_graphicsContext->CreateShaderResources(skyboxResourcesDesc);
    g_skyboxTexShaderResources->BindTextureCube(g_textureCubes["skybox"], 0);
#elif USE_DX11
	ShaderResourcesLayout::Descriptor skyboxResourceLayoutDesc;
	skyboxResourceLayoutDesc.bindings = {
		{ 0, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
		{ 0, ResourceType::TextureCube, ShaderStage::Pixel, 1 }
	};

	g_skyboxShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(skyboxResourceLayoutDesc);

	ShaderResources::Descriptor skyboxResourcesDesc;
	skyboxResourcesDesc.layout = g_skyboxShaderResourcesLayout;

	g_skyboxShaderResources = g_graphicsContext->CreateShaderResources(skyboxResourcesDesc);
    g_skyboxShaderResources->BindConstantBuffer(g_cameraCB, 0);
    g_skyboxShaderResources->BindTextureCube(g_textureCubes["skybox"], 0);
#endif
}

void InitSkyboxGraphicsPipeline() {
    GraphicsShader::Descriptor skyboxShaderDesc;
#if USE_VULKAN
    skyboxShaderDesc.vertexShaderFile = "./assets/shaders/sky.vert.spv";
    skyboxShaderDesc.vertexShaderEntry = "main";
    skyboxShaderDesc.pixelShaderFile = "./assets/shaders/sky.frag.spv";
    skyboxShaderDesc.pixelShaderEntry = "main";
#elif USE_DX11
    skyboxShaderDesc.vertexShaderFile = "./assets/shaders/sky.fx";
    skyboxShaderDesc.vertexShaderEntry = "VSMain";
    skyboxShaderDesc.pixelShaderFile = "./assets/shaders/sky.fx";
    skyboxShaderDesc.pixelShaderEntry = "PSMain";
#endif

    auto skyboxShader = g_graphicsContext->CreateGraphicsShader(skyboxShaderDesc);

    g_skyboxPipeline = g_graphicsContext->CreateGraphicsPipeline();
    g_skyboxPipeline->SetShader(skyboxShader);
#if USE_VULKAN
    g_skyboxPipeline->SetShaderResourcesLayouts({ g_skyboxShaderResourcesLayout, g_skyboxTexShaderResourcesLayout });
#elif USE_DX11
    g_skyboxPipeline->SetShaderResourcesLayouts({ g_skyboxShaderResourcesLayout });
#endif
    g_skyboxPipeline->SetDepthTest(DepthTest::LessEqual, false);
    g_skyboxPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void InitObjectBuffers() {
    StructuredBuffer::Descriptor structuredBufferDesc;
    structuredBufferDesc.memProperty = MemoryProperty::Dynamic;
    structuredBufferDesc.elmSize = sizeof(glm::mat4);
	structuredBufferDesc.bufferSize = sizeof(glm::mat4) * 100;
    structuredBufferDesc.bufferUsages = BufferUsage::ShaderResource;

    g_modelMatricesSB = g_graphicsContext->CreateStructuredBuffer(structuredBufferDesc);
}

void InitObjectShaderResources() {
#if USE_VULKAN
    ShaderResourcesLayout::Descriptor shaderResourceLayoutDesc;
    shaderResourceLayoutDesc.bindings = {
        { 0, ResourceType::ConstantBuffer, ShaderStage::Vertex | ShaderStage::Pixel, 1 },
        { 1, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
        { 2, ResourceType::StructuredBuffer, ShaderStage::Vertex, 1 },
		{ 3, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 }
    };

    g_objShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

    ShaderResources::Descriptor shaderResourcesDesc;
    shaderResourcesDesc.layout = g_objShaderResourcesLayout;

    g_objShaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    g_objShaderResources->BindConstantBuffer(g_cameraCB, 0);
    g_objShaderResources->BindConstantBuffer(g_lightCB, 1);
    g_objShaderResources->BindStructuredBuffer(g_modelMatricesSB, 2);
	g_objShaderResources->BindConstantBuffer(g_materialCB, 3);

    shaderResourceLayoutDesc.bindings = {
        { 0, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
		{ 1, ResourceType::Texture2D, ShaderStage::Pixel, 1 }
    };

    g_objTexShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

    g_objTexShaderResourcesPerFrame.resize(g_graphicsContext->GetMainFramebuffersCount());

#elif USE_DX11
    ShaderResourcesLayout::Descriptor shaderResourceLayoutDesc;
    shaderResourceLayoutDesc.bindings = {
        { 0, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
        { 1, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
        { 0, ResourceType::StructuredBuffer, ShaderStage::Vertex, 1 },
	    { 1, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
    };

    g_objShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

	ShaderResources::Descriptor shaderResourcesDesc;
	shaderResourcesDesc.layout = g_objShaderResourcesLayout;

	g_objShaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    g_objShaderResources->BindConstantBuffer(g_cameraCB, 0);
    g_objShaderResources->BindConstantBuffer(g_lightCB, 1);
    g_objShaderResources->BindStructuredBuffer(g_modelMatricesSB, 0);
    g_objShaderResources->BindTexture2D(g_textures["haus"], 1);
#endif
}

void InitObjectGraphicsPipeline() {
    GraphicsShader::Descriptor shaderDesc;
#if USE_VULKAN
    shaderDesc.vertexShaderFile = "./assets/shaders/shader.vert.spv";
    shaderDesc.vertexShaderEntry = "main";
    shaderDesc.pixelShaderFile = "./assets/shaders/shader.frag.spv";
    shaderDesc.pixelShaderEntry = "main";
#elif USE_DX11
	shaderDesc.vertexShaderFile = "./assets/shaders/shader.fx";
	shaderDesc.vertexShaderEntry = "VSMain";
	shaderDesc.pixelShaderFile = "./assets/shaders/shader.fx";
	shaderDesc.pixelShaderEntry = "PSMain";
#endif

    auto graphicsShader = g_graphicsContext->CreateGraphicsShader(shaderDesc);

    VertexInputLayout::Descriptor vertexInputLayoutDesc;
    vertexInputLayoutDesc.binding = 0;
    vertexInputLayoutDesc.vertexInputRate = VertexInputRate::Vertex;
    vertexInputLayoutDesc.inputElements = {
        { "POSITION", ElementType::Float, 3 },
        { "COLOR", ElementType::Float, 4 },
        { "TEXCOORD", ElementType::Float, 2 },
        { "NORMAL", ElementType::Float, 3 }
    };

    auto vertexInputLayout = g_graphicsContext->CreateVertexInputLayout(vertexInputLayoutDesc);

    g_objPipeline = g_graphicsContext->CreateGraphicsPipeline();
#if USE_VULKAN
	g_objPipeline->SetShaderResourcesLayouts({ g_objShaderResourcesLayout, g_objTexShaderResourcesLayout });
#elif USE_DX11
    g_objPipeline->SetShaderResourcesLayouts({ g_objShaderResourcesLayout });
#endif
    g_objPipeline->SetShader(graphicsShader);
    g_objPipeline->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    g_objPipeline->SetVertexInputLayouts({ vertexInputLayout });
    g_objPipeline->SetDepthTest(DepthTest::Less, true);
    g_objPipeline->SetCullMode(CullMode::Back);
    g_objPipeline->SetFillMode(FillMode::Solid);
    g_objPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

Ref<ShaderResources> GetObjectTextureShaderResources(uint32_t frameIndex) {
	if (g_objTexShaderResourcesUsed >= g_objTexShaderResourcesPerFrame[frameIndex].size()) {
		ShaderResources::Descriptor shaderResourcesDesc;
		shaderResourcesDesc.layout = g_objTexShaderResourcesLayout;

		auto shaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
		g_objTexShaderResourcesPerFrame[frameIndex].push_back(shaderResources);
	}

	return g_objTexShaderResourcesPerFrame[frameIndex][g_objTexShaderResourcesUsed++];
}

void World_Render() {
    auto& commandQueue = g_graphicsContext->GetCommandQueue();

    g_objTexShaderResourcesUsed = 0;

    int32_t width, height;
    g_context->GetFrameBufferSize(width, height);

	auto& lightCube = AddCubeObject();
	lightCube.scale = vec3(0.1f);

#if POINT_LIGHTING
	g_lightConstants.position = vec3(sin(Time::GetTime()), 0.f, cos(Time::GetTime())) * 2.f;

	lightCube.position = g_lightConstants.position;
#elif SPOT_LIGHTING
	//g_lightConstants.direction = vec3(sin(Time::GetTime()), 0.0f, cos(Time::GetTime()));

	lightCube.position = g_lightConstants.position;
#endif
	g_lightCB->Update(&g_lightConstants, sizeof(LightConstants));

    g_cameraConstants.view_matrix = ViewMatrix(vec3(0.f, 0.f, -5.f), vec3(0.f));
    g_cameraConstants.projection_matrix = Perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
    g_cameraConstants.view_projection_matrix = g_cameraConstants.projection_matrix * g_cameraConstants.view_matrix;
	g_cameraConstants.world_position = vec3(0.f, 0.f, -5.f);
    g_cameraCB->Update(&g_cameraConstants, sizeof(CameraConstants));

    commandQueue.SetPipeline(g_skyboxPipeline);
#if USE_VULKAN
    commandQueue.SetShaderResources({ g_skyboxShaderResources, g_skyboxTexShaderResources });
#elif USE_DX11
    commandQueue.SetShaderResources({ g_skyboxShaderResources });
#endif
    commandQueue.Draw(6);

    commandQueue.SetPipeline(g_objPipeline);

    std::vector<glm::mat4> modelMatrices(g_cubeObjects.size());
	for (int32_t i = 0; i < g_cubeObjects.size(); ++i) {
		const auto& object = g_cubeObjects[i];
		modelMatrices[i] = ModelMatrix(object.position, object.rotation, object.scale);
	}

	g_modelMatricesSB->Update(modelMatrices.data(), sizeof(glm::mat4) * modelMatrices.size());

	auto cubeMesh = g_meshes["cube"];
    commandQueue.SetVertexBuffers({ cubeMesh->vertexBuffer });

    for (const auto& subMesh : cubeMesh->subMeshes) {
        auto material = cubeMesh->materials[subMesh.materialIndex];
        auto objTexResources = GetObjectTextureShaderResources(commandQueue.GetCurrentFrameIndex());

        MaterialConstants materialConstants;
        materialConstants.texture_binding_flags = 0;
        materialConstants.diffuseColor = material->diffuseColor;
        materialConstants.specularColor = material->specularColor;
        materialConstants.shininess = material->shininess;

        if (material->diffuseTexture) {
            materialConstants.texture_binding_flags |= static_cast<uint32_t>(TextureBindingFlag::Diffuse);
            objTexResources->BindTexture2D(material->diffuseTexture, 0);
        }

        if (material->specularTexture) {
            materialConstants.texture_binding_flags |= static_cast<uint32_t>(TextureBindingFlag::Specular);
            objTexResources->BindTexture2D(material->specularTexture, 1);
        }

        g_materialCB->Update(&materialConstants, sizeof(MaterialConstants));

#if USE_VULKAN
        commandQueue.SetShaderResources({ g_objShaderResources, objTexResources });
#elif USE_DX11
        commandQueue.SetShaderResources({ g_objShaderResources });
#endif
		commandQueue.DrawIndexedInstanced(cubeMesh->indexBuffer, subMesh.indexCount, g_cubeObjects.size(), subMesh.indexOffset, subMesh.vertexOffset);
    }

    g_cubeObjects.pop_back();
}

void World_Cleanup() {
    g_cubeObjects.clear();
    g_objPipeline.reset();
    g_skyboxPipeline.reset();
    g_objShaderResources.reset();
    g_objShaderResourcesLayout.reset();
	g_objTexShaderResourcesPerFrame.clear();
#if USE_VULKAN
    g_objTexShaderResourcesLayout.reset();
    g_skyboxShaderResources.reset();
    g_skyboxShaderResourcesLayout.reset();
    g_skyboxTexShaderResources.reset();
    g_skyboxTexShaderResourcesLayout.reset();
#elif USE_DX11
	g_skyboxShaderResources.reset();
	g_skyboxShaderResourcesLayout.reset();
	g_objShaderResources.reset();
	g_objShaderResourcesLayout.reset();
#endif
    g_modelMatricesSB.reset();
	g_materialCB.reset();
    g_lightCB.reset();
    g_cameraCB.reset();
    g_meshes.clear();
    g_textureCubes.clear();
    g_materials.clear();
    g_textures.clear();
    g_graphicsContext.reset();
    g_context.reset();

    Log::Info("World resources cleaned up successfully.");

    Log::Cleanup();
}

Object& AddCubeObject() {
    Object object;
    object.position = glm::vec3(0.f);
    object.rotation = glm::vec3(0.f);
    object.scale = glm::vec3(1.f);

    g_cubeObjects.push_back(object);

	return g_cubeObjects.back();
}

std::vector<uint8_t> GenerateTextureCubeData(Image& left, Image& right, Image& top, Image& bottom, Image& front, Image& back) {
    std::vector<uint8_t> textureData;

    uint32_t width = left.Width();
    uint32_t height = left.Height();
    uint32_t channels = left.Channels();

    Image* images[6];

    images[0] = &right; // +X (Right)
    images[1] = &left;  // -X (Left)
    images[2] = &top;   // +Y (Top)
    images[3] = &bottom;// -Y (Bottom)
    images[4] = &front; // +Z (Front)
    images[5] = &back;  // -Z (Back)

    textureData.resize(width * height * 6 * channels);

    uint32_t stride = width * channels * 6;
    for (int i = 0; i < 6; ++i) {
        Image& img = *images[i];
        const auto& imgData = img.Data();
        FASSERT(imgData.size() == width * height * channels, "Image data size does not match expected size for texture cube face.");
        std::copy(imgData.begin(), imgData.end(), textureData.begin() + (i * width * height * channels));
    }
    
    return textureData;
}




