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
#include "Input/Input.h"
#include "Log/Log.h"

Ref<PlatformContext> g_context;
EventDispatcher g_eventDispatcher;
Ref<GraphicsContext> g_graphicsContext;
Ref<EngineCamera> g_camera;

std::vector<Object> g_objects;
std::unordered_map<std::string, Ref<Texture2D>> g_textures;
std::unordered_map<std::string, Ref<TextureCube>> g_textureCubes;
std::unordered_map<std::string, Ref<Mesh>> g_meshes;
std::unordered_map<std::string, Ref<Material>> g_materials;

Ref<VertexInputLayout> g_texturedVertexInputLayout;

#if USE_VULKAN
const uint32_t camersConstantsCBBinding = 0;
const uint32_t lightConstantsCBBinding = 1;
const uint32_t materialConstantsCBBinding = 3;
const uint32_t instanceDataSBBinding = 2;
const uint32_t directionalLightSBBinding = 4;
const uint32_t pointLightSBBinding = 5;
const uint32_t spotLightSBBinding = 6;
const uint32_t diffuseTextureBinding = 0;
const uint32_t specularTextureBinding = 1;
#elif USE_DX11
const uint32_t camersConstantsCBBinding = 0;
const uint32_t lightConstantsCBBinding = 1;
const uint32_t materialConstantsCBBinding = 2;
const uint32_t instanceDataSBBinding = 0;
const uint32_t directionalLightSBBinding = 1;
const uint32_t pointLightSBBinding = 2;
const uint32_t spotLightSBBinding = 3;
const uint32_t diffuseTextureBinding = 4;
const uint32_t specularTextureBinding = 5;
#endif

Ref<ConstantBuffer> g_cameraCB;
Ref<ConstantBuffer> g_lightCB;
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
#elif USE_DX11
Ref<ShaderResourcesLayout> g_skyboxShaderResourcesLayout;
Ref<ShaderResources> g_skyboxShaderResources;
#endif
Ref<ShaderResourcesLayout> g_objShaderResourcesLayout;
Ref<ShaderResources> g_objShaderResources;
Ref<ShaderResourcesLayout> g_objDynamicShaderResourcesLayout;

std::vector<std::vector<Ref<ShaderResources>>> g_objDynamicShaderResourcesPerFrame;
uint32_t g_objDynamicShaderResourcesUsed;

std::vector<std::vector<Ref<ConstantBuffer>>> g_objMaterialCBsPerFrame;
uint32_t g_objMaterialCBUsed;

std::vector<std::vector<Ref<StructuredBuffer>>> g_objInstanceSBsPerFrame;
uint32_t g_objInstanceSBUsed;

CameraConstants g_cameraConstants;
LightConstants g_lightConstants;

void InitAssets();

void InitBaseResources();

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

	g_camera = CreateRef<EngineCamera>();
	g_camera->SetAspectRatio(static_cast<float>(windowWidth) / windowHeight);

    g_eventDispatcher.Register<WindowResizeEvent>([](const WindowResizeEvent& event) { g_graphicsContext->Resize(event.frameBufferWidth, event.frameBufferHeight); }, 0);
    g_eventDispatcher.Register<KeyPressEvent>([](const KeyPressEvent& event) { Input::OnKeyPress(event.key); }, 0);
    g_eventDispatcher.Register<KeyReleaseEvent>([](const KeyReleaseEvent& event) { Input::OnKeyRelease(event.key); }, 0);
    g_eventDispatcher.Register<MouseMoveEvent>([](const MouseMoveEvent& event) { Input::OnMouseMove(event.x, event.y); }, 0);
    g_eventDispatcher.Register<MousePressEvent>([](const MousePressEvent& event) { Input::OnMousePress(event.button); }, 0);
    g_eventDispatcher.Register<MouseReleaseEvent>([](const MouseReleaseEvent& event) { Input::OnMouseRelease(event.button); }, 0);
    g_eventDispatcher.Register<MouseScrollEvent>([](const MouseScrollEvent& event) { Input::OnMouseScroll(event.xOffset, event.yOffset); }, 0);

    g_cameraConstants.view_matrix = ViewMatrix(vec3(0.f, 0.f, -5.f), vec3(0.f));
    g_cameraConstants.projection_matrix = Perspective(glm::radians(45.0f), static_cast<float>(windowWidth) / windowHeight, 0.1f, 100.0f);

    InitAssets();
    InitBaseResources();
    InitSkyboxShaderResources();
    InitSkyboxGraphicsPipeline();
    InitObjectBuffers();
    InitObjectShaderResources();
    InitObjectGraphicsPipeline();
}

void LoadTexture(const char* filePath, const char* key) {
    Image image(filePath, 4);

    if (!image.IsValid()) {
        Log::Error("Failed to load texture: %s", filePath);
        return;
    }

    Texture2D::Descriptor textureDesc;
    textureDesc.width = image.Width();
    textureDesc.height = image.Height();
    textureDesc.data = image.Data().data();
    textureDesc.memProperty = MemoryProperty::Static;
    textureDesc.texUsages = TextureUsage::ShaderResource;
    textureDesc.format = PixelFormat::RGBA8;
	textureDesc.mipLevels = GetMaxMipLevels(textureDesc.width, textureDesc.height);
    textureDesc.shaderStages = ShaderStage::Pixel;
    Ref<Texture2D> texture = g_graphicsContext->CreateTexture2D(textureDesc);

    g_textures[key] = texture;
}

void LoadPrimitiveModel(const std::vector<TexturedVertex>& vertices, const std::vector<uint32_t>& indices, const char* key) {
    Ref<Mesh> mesh = CreateRef<Mesh>();

    VertexBuffer::Descriptor vertexBufferDesc;
    vertexBufferDesc.memProperty = MemoryProperty::Static;
    vertexBufferDesc.elmSize = sizeof(TexturedVertex);
    vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * vertices.size();
    vertexBufferDesc.initialData = vertices.data();

    mesh->vertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

    IndexBuffer::Descriptor indexBufferDesc;
    indexBufferDesc.memProperty = MemoryProperty::Static;
    indexBufferDesc.bufferSize = sizeof(uint32_t) * indices.size();
    indexBufferDesc.initialData = indices.data();

    mesh->indexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

    SubMesh subMesh;
    subMesh.vertexOffset = 0;
    subMesh.indexOffset = 0;
    subMesh.indexCount = indices.size();

    mesh->subMeshes.push_back(subMesh);
    mesh->materials.push_back(g_materials["default"]);

    g_meshes[key] = mesh;
}

void LoadModel(const char* filePath, float scale, const char* key) {
    Ref<Mesh> mesh = CreateRef<Mesh>();
    Model model(filePath, scale);

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
    Texture2D::Descriptor textureDesc;
    textureDesc.width = 1;
	textureDesc.height = 1;
	textureDesc.memProperty = MemoryProperty::Static;
	textureDesc.texUsages = TextureUsage::ShaderResource;
	textureDesc.format = PixelFormat::RGBA8;
	textureDesc.mipLevels = 1;
	textureDesc.shaderStages = ShaderStage::Pixel;

	g_textures["dummy"] = g_graphicsContext->CreateTexture2D(textureDesc);

    LoadTexture("assets/textures/grass.png", "grass");
    LoadTexture("assets/textures/window.png", "window");
	LoadTexture("assets/textures/haus.jpg", "haus");
	LoadTexture("assets/textures/container2.png", "container2");
	LoadTexture("assets/textures/container2_specular.png", "container2_specular");

    Ref<Material> defaultMaterial = CreateRef<Material>();
    defaultMaterial->diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
    defaultMaterial->specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    defaultMaterial->shininess = 32.0f;

    g_materials["default"] = defaultMaterial;

    std::vector<TexturedVertex> quadVertices;
    std::vector<uint32_t> quadIndices;

    GenerateQuad([&](const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& binormal) {
        TexturedVertex vertex;
        vertex.position = position;
        vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex.texCoord = texCoord;
        vertex.normal = normal;
        quadVertices.push_back(vertex);
    }, quadIndices);

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

	std::vector<TexturedVertex> sphereVertices;
	std::vector<uint32_t> sphereIndices;
	GenerateSphere(
		[&](const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& binormal) {
			TexturedVertex vertex;
			vertex.position = position;
			vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			vertex.texCoord = texCoord;
			vertex.normal = normal;
			sphereVertices.push_back(vertex);
		},
		sphereIndices,
        16,
        16,
        0.5f
	);

    LoadPrimitiveModel(quadVertices, quadIndices, "quad");
	LoadPrimitiveModel(cubeVertices, cubeIndices, "cube");
	LoadPrimitiveModel(sphereVertices, sphereIndices, "sphere");
    //LoadModel("assets/models/girl.obj", 1.0f, "girl");
	//LoadModel("assets/models/survival-guitar-backpack/backpack.obj", 1.0f, "survival_backpack");
    LoadModel("assets/models/Sponza/Sponza.gltf", 0.05f, "sponza");

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

void InitBaseResources() {
    VertexInputLayout::Descriptor vertexInputLayoutDesc;
    vertexInputLayoutDesc.binding = 0;
    vertexInputLayoutDesc.vertexInputRate = VertexInputRate::Vertex;
    vertexInputLayoutDesc.inputElements = {
        { "POSITION", ElementType::Float, 3 },
        { "COLOR", ElementType::Float, 4 },
        { "TEXCOORD", ElementType::Float, 2 },
        { "NORMAL", ElementType::Float, 3 }
    };

    g_texturedVertexInputLayout = g_graphicsContext->CreateVertexInputLayout(vertexInputLayoutDesc);

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
    g_skyboxPipeline->SetDepthTest(CompareOp::LessEqual, false);
    g_skyboxPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void InitObjectBuffers() {
	g_objMaterialCBsPerFrame.resize(g_graphicsContext->GetMainFramebuffersCount());
	g_objInstanceSBsPerFrame.resize(g_graphicsContext->GetMainFramebuffersCount());
}

void InitObjectShaderResources() {
    ShaderResourcesLayout::Descriptor shaderResourceLayoutDesc;
    shaderResourceLayoutDesc.bindings = {
        { camersConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Vertex | ShaderStage::Pixel, 1 },
        { lightConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
		{ directionalLightSBBinding, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
		{ pointLightSBBinding, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
		{ spotLightSBBinding, ResourceType::StructuredBuffer, ShaderStage::Pixel, 1 },
    };

    g_objShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

	ShaderResources::Descriptor shaderResourcesDesc;
	shaderResourcesDesc.layout = g_objShaderResourcesLayout;

	g_objShaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    g_objShaderResources->BindConstantBuffer(g_cameraCB, camersConstantsCBBinding);
    g_objShaderResources->BindConstantBuffer(g_lightCB, lightConstantsCBBinding);
	g_objShaderResources->BindStructuredBuffer(g_directionalLightSB, directionalLightSBBinding);
	g_objShaderResources->BindStructuredBuffer(g_pointLightSB, pointLightSBBinding);
	g_objShaderResources->BindStructuredBuffer(g_spotLightSB, spotLightSBBinding);

	shaderResourceLayoutDesc.bindings = {
        { diffuseTextureBinding, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
        { specularTextureBinding, ResourceType::Texture2D, ShaderStage::Pixel, 1 },
        { materialConstantsCBBinding, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
        { instanceDataSBBinding, ResourceType::StructuredBuffer, ShaderStage::Vertex, 1 },
	};

    g_objDynamicShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

    g_objDynamicShaderResourcesPerFrame.resize(g_graphicsContext->GetMainFramebuffersCount());
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

    g_objPipeline = g_graphicsContext->CreateGraphicsPipeline();
	g_objPipeline->SetShaderResourcesLayouts({ g_objShaderResourcesLayout, g_objDynamicShaderResourcesLayout });
    g_objPipeline->SetShader(graphicsShader);
    g_objPipeline->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    g_objPipeline->SetVertexInputLayouts({ g_texturedVertexInputLayout });
    g_objPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
    g_objPipeline->EnableBlendMode(0, true);
    g_objPipeline->SetBlendMode(0, BlendMode::Alpha);
}

Ref<ShaderResources> GetObjDynamicShaderResources(uint32_t frameIndex) {
	if (g_objDynamicShaderResourcesUsed >= g_objDynamicShaderResourcesPerFrame[frameIndex].size()) {
		ShaderResources::Descriptor shaderResourcesDesc;
		shaderResourcesDesc.layout = g_objDynamicShaderResourcesLayout;

		auto shaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
		g_objDynamicShaderResourcesPerFrame[frameIndex].push_back(shaderResources);
	}

	return g_objDynamicShaderResourcesPerFrame[frameIndex][g_objDynamicShaderResourcesUsed++];
}

Ref<ConstantBuffer> GetObjectMaterialCB(uint32_t frameIndex) {
	if (g_objMaterialCBUsed >= g_objMaterialCBsPerFrame[frameIndex].size()) {
		ConstantBuffer::Descriptor materialCBDesc;
		materialCBDesc.memProperty = MemoryProperty::Dynamic;
		materialCBDesc.bufferSize = sizeof(MaterialConstants);
		materialCBDesc.initialData = nullptr;

		auto materialCB = g_graphicsContext->CreateConstantBuffer(materialCBDesc);
		g_objMaterialCBsPerFrame[frameIndex].push_back(materialCB);
	}

	return g_objMaterialCBsPerFrame[frameIndex][g_objMaterialCBUsed++];
}

Ref<StructuredBuffer> GetObjectInstanceSB(uint32_t frameIndex) {
	if (g_objInstanceSBUsed >= g_objInstanceSBsPerFrame[frameIndex].size()) {
		StructuredBuffer::Descriptor structuredBufferDesc;
		structuredBufferDesc.memProperty = MemoryProperty::Dynamic;
		structuredBufferDesc.elmSize = sizeof(InstanceData);
		structuredBufferDesc.bufferSize = sizeof(InstanceData) * 1;
		structuredBufferDesc.bufferUsages = BufferUsage::ShaderResource;

		auto instanceBuffer = g_graphicsContext->CreateStructuredBuffer(structuredBufferDesc);
		g_objInstanceSBsPerFrame[frameIndex].push_back(instanceBuffer);
	}

	return g_objInstanceSBsPerFrame[frameIndex][g_objInstanceSBUsed++];
}

void World_Update() {
    g_objDynamicShaderResourcesUsed = 0;
    g_objMaterialCBUsed = 0;
    g_objInstanceSBUsed = 0;

    int32_t width, height;
    g_context->GetFrameBufferSize(width, height);

    // NOTE: Gather light datas
    DirectionalLight directionalLight;
    directionalLight.direction = Forward;
    directionalLight.ambient = glm::vec3(0.2f);
    directionalLight.diffuse = glm::vec3(0.8f);
    directionalLight.specular = glm::vec3(1.0f);

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

    const float aspectRatio = static_cast<float>(width) / height;
    g_camera->SetAspectRatio(aspectRatio);

    const vec2 nearFar = g_camera->GetNearFarClip();

    g_cameraConstants.near_plane = nearFar.x;
    g_cameraConstants.far_plane = nearFar.y;
    g_cameraConstants.view_matrix = g_camera->GetViewMatrix();
    g_cameraConstants.projection_matrix = g_camera->GetProjectionMatrix();
    g_cameraConstants.view_projection_matrix = g_cameraConstants.projection_matrix * g_cameraConstants.view_matrix;
    g_cameraConstants.world_position = g_camera->GetPosition();
    g_cameraCB->Update(&g_cameraConstants, sizeof(CameraConstants));
}

void World_Render() {
    auto& commandQueue = g_graphicsContext->GetCommandQueue();

    commandQueue.SetPipeline(g_skyboxPipeline);
    commandQueue.ResetVertexBuffers();
#if USE_VULKAN
    commandQueue.SetShaderResources({ g_skyboxShaderResources, g_skyboxTexShaderResources });
#elif USE_DX11
    commandQueue.SetShaderResources({ g_skyboxShaderResources });
#endif
    commandQueue.Draw(6);

    commandQueue.SetPipeline(g_objPipeline);

    struct RenderObject {
        Ref<VertexBuffer> vertexBuffer;
        Ref<IndexBuffer> indexBuffer;
        Ref<Material> material;

        mat4 modelMatrix;
        mat4 invModelMatrix;

        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
    };

    std::vector<RenderObject> renderObjects;

    for (const auto& obj : g_objects) {
        if (obj.HasComponent<StaticMeshComponent>()) {
            auto comp = obj.GetComponent<StaticMeshComponent>();

            for (uint32_t i = 0; i < comp->mesh->subMeshes.size(); i++) {
                auto& subMesh = comp->mesh->subMeshes[i];
                auto material = comp->mesh->materials[i];

                RenderObject renderObj;
                renderObj.vertexBuffer = comp->mesh->vertexBuffer;
                renderObj.indexBuffer = comp->mesh->indexBuffer;
                renderObj.material = material;
                renderObj.modelMatrix = ModelMatrix(obj.position, obj.rotation, obj.scale);
                renderObj.invModelMatrix = glm::inverse(renderObj.modelMatrix);
                renderObj.vertexOffset = subMesh.vertexOffset;
                renderObj.indexOffset = subMesh.indexOffset;
                renderObj.indexCount = subMesh.indexCount;

                renderObjects.push_back(renderObj);
            }
        }
    }

	for (const auto& obj : renderObjects) {
        commandQueue.SetVertexBuffers({ obj.vertexBuffer });

		auto objInstanceBuffer = GetObjectInstanceSB(commandQueue.GetCurrentFrameIndex());

        InstanceData instanceData;
		instanceData.model_matrix = obj.modelMatrix;
		instanceData.inv_model_matrix = obj.invModelMatrix;

        objInstanceBuffer->Update(&instanceData, sizeof(InstanceData));

        auto objDynamicResources = GetObjDynamicShaderResources(commandQueue.GetCurrentFrameIndex());
        auto objMaterialCB = GetObjectMaterialCB(commandQueue.GetCurrentFrameIndex());

        objDynamicResources->BindStructuredBuffer(objInstanceBuffer, instanceDataSBBinding);
        objDynamicResources->BindConstantBuffer(objMaterialCB, materialConstantsCBBinding);

        MaterialConstants materialConstants;
        materialConstants.texture_binding_flags = 0;
        materialConstants.diffuseColor = obj.material->diffuseColor;
        materialConstants.specularColor = obj.material->specularColor;
        materialConstants.shininess = obj.material->shininess;

        if (obj.material->diffuseTexture) {
            materialConstants.texture_binding_flags |= static_cast<uint32_t>(TextureBindingFlag::Diffuse);
            objDynamicResources->BindTexture2D(obj.material->diffuseTexture, diffuseTextureBinding);
        }
        else {
            objDynamicResources->BindTexture2D(g_textures["dummy"], diffuseTextureBinding);
        }

        if (obj.material->specularTexture) {
            materialConstants.texture_binding_flags |= static_cast<uint32_t>(TextureBindingFlag::Specular);
            objDynamicResources->BindTexture2D(obj.material->specularTexture, specularTextureBinding);
        }
        else {
            objDynamicResources->BindTexture2D(g_textures["dummy"], specularTextureBinding);
        }

        objMaterialCB->Update(&materialConstants, sizeof(MaterialConstants));

        commandQueue.SetShaderResources({ g_objShaderResources, objDynamicResources });
        commandQueue.DrawIndexed(obj.indexBuffer, obj.indexCount, obj.indexOffset, obj.vertexOffset);
	}
}

void World_Cleanup() {
    g_objects.clear();
    g_objPipeline.reset();
    g_skyboxPipeline.reset();
    g_objShaderResources.reset();
    g_objShaderResourcesLayout.reset();
	g_objDynamicShaderResourcesPerFrame.clear();
#if USE_VULKAN
    g_skyboxShaderResources.reset();
    g_skyboxShaderResourcesLayout.reset();
    g_skyboxTexShaderResources.reset();
    g_skyboxTexShaderResourcesLayout.reset();
#elif USE_DX11
	g_skyboxShaderResources.reset();
	g_skyboxShaderResourcesLayout.reset();
#endif
    g_objDynamicShaderResourcesLayout.reset();
	g_objShaderResources.reset();
	g_objShaderResourcesLayout.reset();
	g_objInstanceSBsPerFrame.clear();
	g_objMaterialCBsPerFrame.clear();
	g_directionalLightSB.reset();
	g_pointLightSB.reset();
	g_spotLightSB.reset();
    g_lightCB.reset();
    g_cameraCB.reset();
    g_texturedVertexInputLayout.reset();
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




