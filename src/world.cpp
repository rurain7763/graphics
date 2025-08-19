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
Ref<ShaderResources> g_objTexShaderResources;
#elif USE_DX11
Ref<ShaderResourcesLayout> g_skyboxShaderResourcesLayout;
Ref<ShaderResources> g_skyboxShaderResources;
Ref<ShaderResourcesLayout> g_objShaderResourcesLayout;
Ref<ShaderResources> g_objShaderResources;
#endif

std::vector<Object> g_objects;
std::unordered_map<std::string, Ref<Texture2D>> g_textures;
std::unordered_map<std::string, Ref<TextureCube>> g_textureCubes;
std::unordered_map<std::string, Ref<Mesh>> g_meshes;

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

    g_lightConstants.intensity = 1.0f;
    g_lightConstants.color = glm::vec3(1.0f, 1.0f, 1.0f);
    g_lightConstants.direction = QRotate(glm::vec3(0.f, glm::radians(75.f), 0.0f), Forward);

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
    Texture2D::Descriptor hausDesc;
    hausDesc.width = hausImg.Width();
    hausDesc.height = hausImg.Height();
    hausDesc.data = hausImg.Data().data();
    hausDesc.memProperty = MemoryProperty::Static;
    hausDesc.texUsages = TextureUsage::ShaderResource;
    hausDesc.format = PixelFormat::RGBA8;
    hausDesc.mipLevels = GetMaxMipLevels(hausDesc.width, hausDesc.height);
	hausDesc.shaderStages = ShaderStage::Pixel;

    g_textures["haus"] = g_graphicsContext->CreateTexture2D(hausDesc);

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

    VertexBuffer::Descriptor vertexBufferDesc;
    vertexBufferDesc.memProperty = MemoryProperty::Static;
    vertexBufferDesc.elmSize = sizeof(TexturedVertex);
    vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * modelVertices.size();
    vertexBufferDesc.initialData = modelVertices.data();

    girlMesh->vertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

    IndexBuffer::Descriptor indexBufferDesc;
    indexBufferDesc.memProperty = MemoryProperty::Static;
    indexBufferDesc.bufferSize = sizeof(uint32_t) * modelIndices.size();
    indexBufferDesc.initialData = modelIndices.data();

    girlMesh->indexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

    for (const auto& mesh : girlModel.GetMeshs()) {
        SubMesh subMesh;
        subMesh.vertexOffset = mesh.vertexStart;
        subMesh.indexOffset = mesh.indexStart;
        subMesh.indexCount = mesh.indexCount;
        girlMesh->subMeshes.push_back(subMesh);
    }

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
        { 0, ResourceType::TextureCube, ShaderStage::Pixel, 1 }
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

	g_skyboxResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(skyboxResourceLayoutDesc);

	ShaderResources::Descriptor skyboxResourcesDesc;
	skyboxResourcesDesc.layout = g_skyboxResourcesLayout;

	g_skyboxResources = g_graphicsContext->CreateShaderResources(skyboxResourcesDesc);
	g_skyboxResources->BindConstantBuffer(g_cameraCB, 0);
	g_skyboxResources->BindTextureCube(g_textureCubes["skybox"], 0);
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
    g_skyboxPipeline->SetShaderResourcesLayouts({ g_skyboxResourcesLayout });
#endif
    g_skyboxPipeline->SetDepthTest(DepthTest::LessEqual, false);
    g_skyboxPipeline->SetBehaviorStates(GraphicsPipeline::Behavior::AutoResizeViewport | GraphicsPipeline::Behavior::AutoResizeScissor);
}

void InitObjectBuffers() {
    StructuredBuffer::Descriptor structuredBufferDesc;
    structuredBufferDesc.memProperty = MemoryProperty::Dynamic;
    structuredBufferDesc.elmSize = sizeof(glm::mat4);
	structuredBufferDesc.bufferSize = sizeof(glm::mat4);
    structuredBufferDesc.bufferUsages = BufferUsage::ShaderResource;

    g_modelMatricesSB = g_graphicsContext->CreateStructuredBuffer(structuredBufferDesc);
}

void InitObjectShaderResources() {
#if USE_VULKAN
    ShaderResourcesLayout::Descriptor shaderResourceLayoutDesc;
    shaderResourceLayoutDesc.bindings = {
        { 0, ResourceType::ConstantBuffer, ShaderStage::Vertex, 1 },
        { 1, ResourceType::ConstantBuffer, ShaderStage::Pixel, 1 },
        { 2, ResourceType::StructuredBuffer, ShaderStage::Vertex, 1 }
    };

    g_objShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

    ShaderResources::Descriptor shaderResourcesDesc;
    shaderResourcesDesc.layout = g_objShaderResourcesLayout;

    g_objShaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    g_objShaderResources->BindConstantBuffer(g_cameraCB, 0);
    g_objShaderResources->BindConstantBuffer(g_lightCB, 1);
    g_objShaderResources->BindStructuredBuffer(g_modelMatricesSB, 2);

    shaderResourceLayoutDesc.bindings = {
        { 0, ResourceType::Texture2D, ShaderStage::Pixel, 1 }
    };

    g_objTexShaderResourcesLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

    shaderResourcesDesc.layout = g_objTexShaderResourcesLayout;

    g_objTexShaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    g_objTexShaderResources->BindTexture2D(g_textures["haus"], 0);
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

void World_Update() {
    int32_t width, height;
    g_context->GetFrameBufferSize(width, height);

    g_cameraConstants.view_matrix = ViewMatrix(vec3(0.f, 0.f, -5.f), vec3(0.f));
    g_cameraConstants.projection_matrix = Perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
    g_cameraConstants.view_projection_matrix = g_cameraConstants.projection_matrix * g_cameraConstants.view_matrix;
    g_cameraCB->Update(&g_cameraConstants, sizeof(CameraConstants));
}

void World_Render() {
    auto& commandQueue = g_graphicsContext->GetCommandQueue();

    commandQueue.SetPipeline(g_skyboxPipeline);
#if USE_VULKAN
    commandQueue.SetShaderResources({ g_skyboxShaderResources, g_skyboxTexShaderResources });
#elif USE_DX11
    commandQueue.SetShaderResources({ g_skyboxShaderResources });
#endif
    commandQueue.Draw(6);

    commandQueue.SetPipeline(g_objPipeline);

#if USE_VULKAN
    commandQueue.SetShaderResources({ g_objShaderResources, g_objTexShaderResources });
#elif USE_DX11
    commandQueue.SetShaderResources({ g_objShaderResources });
#endif

    std::vector<glm::mat4> modelMatrices(1);
    for (const auto& object : g_objects) {
        modelMatrices[0] = ModelMatrix(object.position, object.rotation, object.scale);
        g_modelMatricesSB->Update(modelMatrices.data(), sizeof(glm::mat4) * modelMatrices.size());

        commandQueue.SetVertexBuffers({ object.mesh->vertexBuffer });

        for (const auto& subMesh : object.mesh->subMeshes) {
            commandQueue.DrawIndexed(object.mesh->indexBuffer, subMesh.indexCount, subMesh.indexOffset, subMesh.vertexOffset);
        }
    }
}

void World_Cleanup() {
    g_objects.clear();
    g_objPipeline.reset();
    g_skyboxPipeline.reset();
    g_objShaderResources.reset();
    g_objShaderResourcesLayout.reset();
    g_objTexShaderResources.reset();
    g_objTexShaderResourcesLayout.reset();
    g_skyboxShaderResources.reset();
    g_skyboxShaderResourcesLayout.reset();
    g_skyboxTexShaderResources.reset();
    g_skyboxTexShaderResourcesLayout.reset();
    g_modelMatricesSB.reset();
    g_lightCB.reset();
    g_cameraCB.reset();
    g_meshes.clear();
    g_textureCubes.clear();
    g_textures.clear();
    g_graphicsContext.reset();
    g_context.reset();

    Log::Info("World resources cleaned up successfully.");

    Log::Cleanup();
}

void AddObject(const char* meshKey) {
    auto it = g_meshes.find(meshKey);
    if (it != g_meshes.end()) {
        Object object;
        object.position = glm::vec3(0.f);
        object.rotation = glm::vec3(0.f);
        object.scale = glm::vec3(1.f);
        object.mesh = it->second;

        g_objects.push_back(object);
    } else {
        Log::Error("Mesh not found: %s", meshKey);
    }
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




