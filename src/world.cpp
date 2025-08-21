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
Ref<StructuredBuffer> g_directionalLightSB;
Ref<StructuredBuffer> g_pointLightSB;
Ref<StructuredBuffer> g_spotLightSB;
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

std::vector<Object> g_objects;
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

    InitAssets();
    InitBaseBuffers();
    InitSkyboxShaderResources();
    InitSkyboxGraphicsPipeline();
    InitObjectBuffers();
    InitObjectShaderResources();
    InitObjectGraphicsPipeline();
}

void LoadModel(const char* filePath, const char* key) {
    Ref<Mesh> mesh = CreateRef<Mesh>();
    Model model(filePath);

    std::vector<TexturedVertex> vertices;
    for (const auto& vertex : model.GetVertices()) {
        TexturedVertex texturedVertex;
        texturedVertex.position = vertex.position;
        texturedVertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        texturedVertex.texCoord = vertex.texCoord;
        texturedVertex.normal = vertex.normal;
        vertices.push_back(texturedVertex);
    }

	VertexBuffer::Descriptor vertexBufferDesc;
    vertexBufferDesc.memProperty = MemoryProperty::Static;
    vertexBufferDesc.elmSize = sizeof(TexturedVertex);
    vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * vertices.size();
    vertexBufferDesc.initialData = vertices.data();

    mesh->vertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

    const std::vector<uint32_t>& indices = model.GetIndices();

	IndexBuffer::Descriptor indexBufferDesc;
    indexBufferDesc.memProperty = MemoryProperty::Static;
    indexBufferDesc.bufferSize = sizeof(uint32_t) * indices.size();
    indexBufferDesc.initialData = indices.data();

    mesh->indexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

    std::unordered_map<Ref<Image>, Ref<Texture2D>> textureCache;
    std::function<Ref<Texture2D>(const Ref<Image>&)> createTexture = [&](const Ref<Image>& image) {
        auto it = textureCache.find(image);
        if (it != textureCache.end()) {
            return it->second;
        }

        Texture2D::Descriptor textureDesc;
        textureDesc.width = image->Width();
        textureDesc.height = image->Height();
        textureDesc.data = image->Data().data();
        textureDesc.memProperty = MemoryProperty::Static;
        textureDesc.texUsages = TextureUsage::ShaderResource;
        textureDesc.format = PixelFormat::RGBA8;
        textureDesc.mipLevels = 1;
        textureDesc.shaderStages = ShaderStage::Pixel;

        Ref<Texture2D> texture = g_graphicsContext->CreateTexture2D(textureDesc);
        textureCache[image] = texture;

        return texture;
    };

	std::unordered_map<uint32_t, Ref<Material>> materialCache;
	std::function<Ref<Material>(uint32_t, const ModelMaterial&)> createMaterial = [&](uint32_t index, const ModelMaterial& modelMaterial) {
		auto it = materialCache.find(index);
		if (it != materialCache.end()) {
			return it->second;
		}

		Ref<Material> material = CreateRef<Material>();
        material->diffuseColor = modelMaterial.baseColor;
		if (modelMaterial.diffuse) {
			material->diffuseTexture = createTexture(modelMaterial.diffuse);
		}
		material->specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
        if (modelMaterial.specular) {
			material->specularTexture = createTexture(modelMaterial.specular);
        }
		material->shininess = 32.0f;
		materialCache[index] = material;

		return material;
	};

    for (const auto& modelSubMesh : model.GetMeshs()) {
        SubMesh subMesh;
        subMesh.vertexOffset = modelSubMesh.vertexStart;
        subMesh.indexOffset = modelSubMesh.indexStart;
        subMesh.indexCount = modelSubMesh.indexCount;
        mesh->subMeshes.push_back(subMesh);

		Ref<Material> material;
        if (modelSubMesh.materialIndex == -1) {
			material = g_materials["default"];
        }
        else {
		    material = createMaterial(modelSubMesh.materialIndex, model.GetMaterialAt(modelSubMesh.materialIndex));
        }

		mesh->materials.push_back(material);
    }

	g_meshes[key] = mesh;
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
	cubeMesh->subMeshes.push_back(subMesh);

    cubeMesh->materials.push_back(defaultMaterial);

	g_meshes["cube"] = cubeMesh;

	LoadModel("assets/models/sphere.obj", "sphere");
    LoadModel("assets/models/girl.obj", "girl");
	LoadModel("assets/models/survival-guitar-backpack/backpack.obj", "survival_backpack");

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

	StructuredBuffer::Descriptor directionalLightDesc;
	directionalLightDesc.memProperty = MemoryProperty::Dynamic;
	directionalLightDesc.elmSize = sizeof(DirectionalLight);
	directionalLightDesc.bufferSize = sizeof(DirectionalLight) * MAX_DIRECTIONAL_LIGHTS;
	directionalLightDesc.bufferUsages = BufferUsage::ShaderResource;    

	g_directionalLightSB = g_graphicsContext->CreateStructuredBuffer(directionalLightDesc);

	StructuredBuffer::Descriptor pointLightDesc;
	pointLightDesc.memProperty = MemoryProperty::Dynamic;
	pointLightDesc.elmSize = sizeof(PointLight);
	pointLightDesc.bufferSize = sizeof(PointLight) * MAX_POINT_LIGHTS;
	pointLightDesc.bufferUsages = BufferUsage::ShaderResource;

	g_pointLightSB = g_graphicsContext->CreateStructuredBuffer(pointLightDesc);

	StructuredBuffer::Descriptor spotLightDesc;
	spotLightDesc.memProperty = MemoryProperty::Dynamic;
	spotLightDesc.elmSize = sizeof(SpotLight);
	spotLightDesc.bufferSize = sizeof(SpotLight) * MAX_SPOT_LIGHTS;
	spotLightDesc.bufferUsages = BufferUsage::ShaderResource;

	g_spotLightSB = g_graphicsContext->CreateStructuredBuffer(spotLightDesc);
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
		{ 3, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
        { 4, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
        { 5, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
        { 6, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
    };

    g_objShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

    ShaderResources::Descriptor shaderResourcesDesc;
    shaderResourcesDesc.layout = g_objShaderResourcesLayout;

    g_objShaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    g_objShaderResources->BindConstantBuffer(g_cameraCB, 0);
    g_objShaderResources->BindConstantBuffer(g_lightCB, 1);
    g_objShaderResources->BindStructuredBuffer(g_modelMatricesSB, 2);
	g_objShaderResources->BindConstantBuffer(g_materialCB, 3);
	g_objShaderResources->BindStructuredBuffer(g_directionalLightSB, 4);
	g_objShaderResources->BindStructuredBuffer(g_pointLightSB, 5);
	g_objShaderResources->BindStructuredBuffer(g_spotLightSB, 6);

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

    // NOTE: Gather light datas
    DirectionalLight directionalLight;
    directionalLight.direction = Forward;
    directionalLight.ambient = glm::vec3(0.1f);
    directionalLight.diffuse = glm::vec3(0.2f);
    directionalLight.specular = glm::vec3(0.f);

    std::vector<PointLight> pointLights(2);
    for (int32_t i = 0; i < pointLights.size(); ++i) {
		if (i == 0) {
			pointLights[i].position = vec3(cos(Time::GetTime()), sin(Time::GetTime()), 0) * 2.0f;
		}
		else if (i == 1) {
			pointLights[i].position = vec3(cos(Time::GetTime()), 0, sin(Time::GetTime())) * 2.0f;
		}
        pointLights[i].constant_attenuation = 1.0f;
        pointLights[i].linear_attenuation = 0.09f;
        pointLights[i].quadratic_attenuation = 0.032f;
        pointLights[i].ambient = glm::vec3(0.2f);
        pointLights[i].diffuse = glm::vec3(0.8f);
        pointLights[i].specular = glm::vec3(1.0f);
    }

    std::vector<SpotLight> spotLights(1);
    for (int32_t i = 0; i < spotLights.size(); ++i) {
        if (i == 0) {
            spotLights[i].position = vec3(0, 0, -5);
			spotLights[i].direction = normalize(vec3(sin(Time::GetTime()), 0, abs(cos(Time::GetTime()))));
        }

        spotLights[i].cutoff_inner_cosine = glm::cos(glm::radians(15.f));
        spotLights[i].cutoff_outer_cosine = glm::cos(glm::radians(17.0f));
        spotLights[i].constant_attenuation = 1.0f;
        spotLights[i].linear_attenuation = 0.09f;
        spotLights[i].quadratic_attenuation = 0.032f;
        spotLights[i].ambient = glm::vec3(0.2f);
        spotLights[i].diffuse = glm::vec3(0.8f);
        spotLights[i].specular = glm::vec3(1.0f);
    }

    g_lightConstants.directional_light_count = 1;
    g_lightConstants.point_light_count = pointLights.size();
    g_lightConstants.spot_light_count = spotLights.size();

    g_directionalLightSB->Update(&directionalLight, sizeof(DirectionalLight) * g_lightConstants.directional_light_count);
    g_pointLightSB->Update(pointLights.data(), sizeof(PointLight) * g_lightConstants.point_light_count);
    g_spotLightSB->Update(spotLights.data(), sizeof(SpotLight) * g_lightConstants.spot_light_count);
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

    std::vector<glm::mat4> modelMatrices(g_objects.size());
	for (int32_t i = 0; i < g_objects.size(); ++i) {
		const auto& object = g_objects[i];
		modelMatrices[i] = ModelMatrix(object.position, object.rotation, object.scale);
	}

	g_modelMatricesSB->Update(modelMatrices.data(), sizeof(glm::mat4) * modelMatrices.size());

	auto mesh = g_meshes["survival_backpack"];
    commandQueue.SetVertexBuffers({ mesh->vertexBuffer });

    for (uint32_t i = 0; i < mesh->subMeshes.size(); i++) {
		auto& subMesh = mesh->subMeshes[i];
        auto material = mesh->materials[i];
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
		commandQueue.DrawIndexedInstanced(mesh->indexBuffer, subMesh.indexCount, g_objects.size(), subMesh.indexOffset, subMesh.vertexOffset);
    }
}

void World_Cleanup() {
    g_objects.clear();
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
	g_directionalLightSB.reset();
	g_pointLightSB.reset();
	g_spotLightSB.reset();
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

Object& AddObject() {
    Object object;
    object.position = glm::vec3(0.f);
    object.rotation = glm::vec3(0.f);
    object.scale = glm::vec3(1.f);

    g_objects.push_back(object);

	return g_objects.back();
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




