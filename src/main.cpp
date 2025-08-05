#include "pch.h"
#include "Log/Log.h"
#include "Event/EventDispatcher.h"
#include "Platform/PlatformContext.h"
#include "Platform/PlatformEvents.h"
#include "Graphics/Vulkan/VkContext.h"
#include "Graphics/Vulkan/VkPipelines.h"
#include "Graphics/Vulkan/VkCommandQueue.h"
#include "Time/Time.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "Model/Model.h"
#include "Graphics/GraphicsFunc.h"

#include <filesystem>

using namespace flaw;

struct PushConstants {
    glm::mat4 model_matrix;
};

struct CameraConstants {
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
    glm::mat4 view_projection_matrix;
};

struct LightConstants {
    glm::vec3 color;
    float intensity;
    glm::vec3 direction;
    float padding;
};

struct ColoredVertex {
    glm::vec3 position;
    glm::vec4 color;
};

struct TexturedVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

std::vector<uint8_t> GenerateTextureCubeData(Ref<GraphicsContext> g_graphicsContext, Image& left, Image& right, Image& top, Image& bottom, Image& front, Image& back) {
    std::vector<uint8_t> textureData;

    uint32_t width = left.Width();
    uint32_t height = left.Height();
    uint32_t channels = left.Channels();

    Image* images[6];

    if (auto vkContext = dynamic_cast<VkContext*>(g_graphicsContext.get())) {
        images[0] = &right; // +X (Right)
        images[1] = &left;  // -X (Left)
        images[2] = &top;   // +Y (Top)
        images[3] = &bottom;// -Y (Bottom)
        images[4] = &front; // +Z (Front)
        images[5] = &back;  // -Z (Back)
    }
    else {
        Log::Error("Unsupported graphics context type for texture cube generation.");
        return textureData;
    }

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

Ref<VkContext> g_graphicsContext;

Ref<ConstantBuffer> g_cameraConstants;

Ref<TextureCube> g_skybox;
Ref<GraphicsShader> g_skyboxShader;
Ref<ShaderResourcesLayout> g_skyboxResourcesLayout0;
Ref<ShaderResourcesLayout> g_skyboxResourcesLayout1;
Ref<ShaderResources> g_skyboxResources0;
Ref<ShaderResources> g_skyboxResources1;
Ref<GraphicsPipeline> g_skyboxPipeline;

void MakeSkyboxResources();
void ReleaseSkyboxResources();

int main() {
    Log::Initialize();
    
    Log::Info("Application started successfully.");
    Log::Info("Current working directory: %s", std::filesystem::current_path().string().c_str());

    EventDispatcher eventDispatcher;

    const int32_t windowWidth = 800;
    const int32_t windowHeight = 600;

    auto context = CreateRef<PlatformContext>("Flaw Application", windowWidth, windowHeight, eventDispatcher);
    g_graphicsContext = CreateRef<VkContext>(*context, windowWidth, windowHeight);

    // Set vulkan specific parameters
    MathParams::InvertYAxis = true;
    ModelParams::LeftHanded = true;

    eventDispatcher.Register<WindowResizeEvent>([&](const WindowResizeEvent& event) {
        g_graphicsContext->Resize(event.frameBufferWidth, event.frameBufferHeight);
    }, 0);

    Image faceImg("./assets/textures/face.jpg", 4);
    Texture2D::Descriptor faceDesc;
    faceDesc.width = faceImg.Width();
    faceDesc.height = faceImg.Height();
    faceDesc.data = faceImg.Data().data();
    faceDesc.usage = UsageFlag::Static;
    faceDesc.bindFlags = BindFlag::ShaderResource;
    faceDesc.format = PixelFormat::RGBA8;

    Ref<Texture2D> faceTexture = g_graphicsContext->CreateTexture2D(faceDesc);

    Image hausImg("./assets/textures/haus.jpg", 4);
    Texture2D::Descriptor hausDesc;
    hausDesc.width = hausImg.Width();
    hausDesc.height = hausImg.Height();
    hausDesc.data = hausImg.Data().data();
    hausDesc.usage = UsageFlag::Static;
    hausDesc.bindFlags = BindFlag::ShaderResource;
    hausDesc.format = PixelFormat::RGBA8;
    hausDesc.mipLevels = GetMaxMipLevels(hausDesc.width, hausDesc.height);

    Ref<Texture2D> hausTexture = g_graphicsContext->CreateTexture2D(hausDesc);

    GraphicsShader::Descriptor shaderDesc;
    shaderDesc.vertexShaderFile = "./assets/shaders/shader.vert.spv";
    shaderDesc.vertexShaderEntry = "main";
    shaderDesc.pixelShaderFile = "./assets/shaders/shader.frag.spv";
    shaderDesc.pixelShaderEntry = "main";

    auto graphicsShader = g_graphicsContext->CreateGraphicsShader(shaderDesc);

    ShaderResourcesLayout::Descriptor shaderResourceLayoutDesc;
    shaderResourceLayoutDesc.bindings = {
        { 0, ResourceType::ConstantBuffer, ShaderCompileFlag::Vertex, 1 },
        { 1, ResourceType::ConstantBuffer, ShaderCompileFlag::Pixel, 1 },
        { 2, ResourceType::StructuredBuffer, ShaderCompileFlag::Vertex, 1 }
    };

    auto shaderResourceLayout = g_graphicsContext->CreateShaderResourcesLayout(shaderResourceLayoutDesc);

    ShaderResourcesLayout::Descriptor textureResourceLayoutDesc;
    textureResourceLayoutDesc.bindings = {
        { 0, ResourceType::Texture2D, ShaderCompileFlag::Pixel, 1 }
    };

    auto textureResourceLayout = g_graphicsContext->CreateShaderResourcesLayout(textureResourceLayoutDesc);

    GraphicsVertexInputLayout::Descriptor vertexInputLayoutDesc;
    vertexInputLayoutDesc.binding = 0;
    vertexInputLayoutDesc.vertexInputRate = VertexInputRate::Vertex;
    vertexInputLayoutDesc.inputElements = {
        { "POSITION", ElementType::Float, 3 },
        { "COLOR", ElementType::Float, 4 },
        { "TEX_COORD", ElementType::Float, 2 },
        { "NORMAL", ElementType::Float, 3 }
    };

    auto vertexInputLayout = g_graphicsContext->CreateVertexInputLayout(vertexInputLayoutDesc);

    Model cubeModel("./assets/models/cube.obj");
    Model sphereModel("./assets/models/sphere.obj");
    Model girlModel("./assets/models/girl.obj");

    Model& currentModel = girlModel; // Change to model you want to render

    std::vector<TexturedVertex> modelVertices;
    for (const auto& vertex : currentModel.GetVertices()) {
        TexturedVertex texturedVertex;
        texturedVertex.position = vertex.position;
        texturedVertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        texturedVertex.texCoord = vertex.texCoord;
        texturedVertex.normal = vertex.normal;
        modelVertices.push_back(texturedVertex);
    }

    std::vector<uint32_t> modelIndices = currentModel.GetIndices();

    VertexBuffer::Descriptor vertexBufferDesc;
    vertexBufferDesc.usage = UsageFlag::Static;
    vertexBufferDesc.elmSize = sizeof(TexturedVertex);
    vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * modelVertices.size();
    vertexBufferDesc.initialData = modelVertices.data();

    auto modelVertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

    IndexBuffer::Descriptor indexBufferDesc;
    indexBufferDesc.usage = UsageFlag::Static;
    indexBufferDesc.bufferSize = sizeof(uint32_t) * modelIndices.size();
    indexBufferDesc.initialData = modelIndices.data();

    auto modelIndexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

    auto graphicsPipeline = std::static_pointer_cast<VkGraphicsPipeline>(g_graphicsContext->CreateGraphicsPipeline());
    graphicsPipeline->AddShaderResourcesLayout(shaderResourceLayout);
    graphicsPipeline->AddShaderResourcesLayout(textureResourceLayout);
    graphicsPipeline->SetShader(graphicsShader);
    graphicsPipeline->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    graphicsPipeline->SetVertexInputLayout(vertexInputLayout);
    graphicsPipeline->SetDepthTest(DepthTest::Less, true);
    graphicsPipeline->SetCullMode(CullMode::Back);
    graphicsPipeline->SetFillMode(FillMode::Solid);
    graphicsPipeline->SetRenderPassLayout(g_graphicsContext->GetMainRenderPassLayout());
    graphicsPipeline->SetBehaviorStates(
        GraphicsPipeline::BehaviorFlag::AutoResizeViewport |
        GraphicsPipeline::BehaviorFlag::AutoResizeScissor
    );

    glm::vec3 cameraPosition = { 0.0f, 0.0f, -5.0f };
    glm::vec3 cameraRotation = { 0.0f, 0.0f, 0.0f };

    // Create Constant Buffers
    g_cameraConstants = g_graphicsContext->CreateConstantBuffer(sizeof(CameraConstants));

    LightConstants lightConstants;
    lightConstants.intensity = 1.0f;
    lightConstants.color = glm::vec3(1.0f, 1.0f, 1.0f);
    lightConstants.direction = QRotate(glm::vec3(0.f, glm::radians(75.f), 0.0f), Forward);
    
    auto lightConstantBuffer = g_graphicsContext->CreateConstantBuffer(sizeof(LightConstants));
    lightConstantBuffer->Update(&lightConstants, sizeof(LightConstants));

    glm::vec3 modelPosition = { 0.0f, 0.0f, 0.0f };
    glm::vec3 modelRotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 modelScale = { 1.0f, 1.0f, 1.0f };

    glm::mat4 modelMatrices = ModelMatrix(modelPosition, modelRotation, modelScale);

    StructuredBuffer::Descriptor structuredBufferDesc;
    structuredBufferDesc.elmSize = sizeof(glm::mat4);
    structuredBufferDesc.count = 1;
    structuredBufferDesc.accessFlags = AccessFlag::Write;
    structuredBufferDesc.bindFlags = BindFlag::ShaderResource;
    structuredBufferDesc.initialData = &modelMatrices;

    auto structuredBuffer = g_graphicsContext->CreateStructuredBuffer(structuredBufferDesc);

    ShaderResources::Descriptor shaderResourcesDesc;
    shaderResourcesDesc.layout = shaderResourceLayout;

    auto shaderResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    shaderResources->BindConstantBuffer(g_cameraConstants, 0);
    shaderResources->BindConstantBuffer(lightConstantBuffer, 1);
    shaderResources->BindStructuredBuffer(structuredBuffer, 2);

    shaderResourcesDesc.layout = textureResourceLayout;

    auto textureResources = g_graphicsContext->CreateShaderResources(shaderResourcesDesc);
    textureResources->BindTexture2D(hausTexture, 0);

    shaderResourcesDesc.layout = nullptr;

    GraphicsRenderPassLayout::Descriptor subFramesRenderPassLayoutDesc;
    subFramesRenderPassLayoutDesc.type = PipelineType::Graphics;
    subFramesRenderPassLayoutDesc.colorAttachments = {
        { g_graphicsContext->GetMainFramebuffer(0)->GetAttachment(0)->GetPixelFormat(), BlendMode::Default, false }
    };
    subFramesRenderPassLayoutDesc.depthStencilAttachment = { 
        g_graphicsContext->GetMainFramebuffer(0)->GetDepthStencilAttachment()->GetPixelFormat() 
    };

    auto subFramesRenderPassLayout = g_graphicsContext->CreateRenderPassLayout(subFramesRenderPassLayoutDesc);

    GraphicsRenderPass::Descriptor subFramesRenderPassDesc;
    subFramesRenderPassDesc.layout = subFramesRenderPassLayout;
    subFramesRenderPassDesc.colorAttachmentOperations = {
        { TextureLayout::Undefined, TextureLayout::Present, AttachmentLoadOp::Clear, AttachmentStoreOp::Store }
    };
    subFramesRenderPassDesc.depthStencilAttachmentOperation = {
        { TextureLayout::Undefined, TextureLayout::DepthStencil, AttachmentLoadOp::Clear, AttachmentStoreOp::Store, AttachmentLoadOp::DontCare, AttachmentStoreOp::DontCare }
    };

    auto subFramesRenderPass = g_graphicsContext->CreateRenderPass(subFramesRenderPassDesc);

    std::vector<Ref<GraphicsFramebuffer>> subFrameBuffers;
    std::vector<Ref<GraphicsRenderPass>> subFramesRenderPasses;

    for (uint32_t i = 0; i < g_graphicsContext->GetMainFramebuffersCount(); ++i) {
        auto framebuffer = g_graphicsContext->GetMainFramebuffer(i);

        GraphicsFramebuffer::Descriptor framebufferDesc;
        framebufferDesc.width = framebuffer->GetWidth();
        framebufferDesc.height = framebuffer->GetHeight();
        framebufferDesc.colorAttachments = { framebuffer->GetAttachment(0) };
        framebufferDesc.colorResizeHandler = [i](uint32_t index, uint32_t width, uint32_t height) {
            return g_graphicsContext->GetMainFramebuffer(i)->GetAttachment(index);
        };
        framebufferDesc.depthStencilAttachment = framebuffer->GetDepthStencilAttachment();
        framebufferDesc.depthStencilResizeHandler = [i](uint32_t width, uint32_t height) {
            return g_graphicsContext->GetMainFramebuffer(i)->GetDepthStencilAttachment();
        };               
        framebufferDesc.renderPassLayout = framebuffer->GetRenderPassLayout();

        subFrameBuffers.push_back(g_graphicsContext->CreateFramebuffer(framebufferDesc));
        subFramesRenderPasses.push_back(subFramesRenderPass);
    }

    MakeSkyboxResources();

    auto& commandQueue = static_cast<VkCommandQueue&>(g_graphicsContext->GetCommandQueue());
    
    Time::Start();
    while (context->PollEvents()) {
        eventDispatcher.PollEvents();

        Time::Update();

        std::string title = "Flaw Application - FPS: " + std::to_string(Time::FPS()) + " | Delta Time: " + std::to_string(Time::DeltaTime() * 1000.0f) + " ms";
        context->SetTitle(title.c_str());

        int32_t width, height;
        g_graphicsContext->GetSize(width, height);

        CameraConstants cameraConstants;
        cameraConstants.view_matrix = ViewMatrix(cameraPosition, cameraRotation);
        cameraConstants.projection_matrix = Perspective(glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f);
        cameraConstants.view_projection_matrix = cameraConstants.projection_matrix * cameraConstants.view_matrix;
        g_cameraConstants->Update(&cameraConstants, sizeof(CameraConstants));

        modelRotation.y += Time::DeltaTime();

        modelMatrices = ModelMatrix(modelPosition, modelRotation, modelScale);
        structuredBuffer->Update(&modelMatrices, sizeof(glm::mat4));

        if (g_graphicsContext->Prepare()) {
            commandQueue.SetFramebuffers(subFrameBuffers, subFramesRenderPasses);
            commandQueue.ResetFramebuffers();

            commandQueue.SetPipeline(g_skyboxPipeline);
            commandQueue.SetShaderResources(g_skyboxResources0, 0);
            commandQueue.SetShaderResources(g_skyboxResources1, 1);
            commandQueue.Draw(6);

            commandQueue.SetPipeline(graphicsPipeline);
            commandQueue.SetShaderResources(shaderResources, 0);
            commandQueue.SetShaderResources(textureResources, 1);
            commandQueue.SetVertexBuffer(modelVertexBuffer);
            
            for (const auto& mesh : currentModel.GetMeshs()) {
                commandQueue.DrawIndexed(modelIndexBuffer, mesh.indexCount, mesh.indexStart, mesh.vertexStart);
            }

            commandQueue.Present();
        }
    }

    ReleaseSkyboxResources();
    subFramesRenderPass.reset();
    subFramesRenderPassLayout.reset();
    subFramesRenderPasses.clear();
    subFrameBuffers.clear();
    shaderResources.reset();
    textureResources.reset();
    structuredBuffer.reset();
    lightConstantBuffer.reset();
    g_cameraConstants.reset();
    graphicsPipeline.reset();
    modelIndexBuffer.reset();
    modelVertexBuffer.reset();
    vertexInputLayout.reset();
    textureResourceLayout.reset();
    shaderResourceLayout.reset();
    graphicsShader.reset();
    hausTexture.reset();
    faceTexture.reset();
    g_graphicsContext.reset();
    context.reset();

    Log::Cleanup();

    Log::Info("Application exited successfully.");

    return 0;
}

void MakeSkyboxResources() {
    Image left("./assets/textures/sky/sky_left.png", 4);
    Image right("./assets/textures/sky/sky_right.png", 4);
    Image top("./assets/textures/sky/sky_top.png", 4);
    Image bottom("./assets/textures/sky/sky_bottom.png", 4);
    Image front("./assets/textures/sky/sky_front.png", 4);
    Image back("./assets/textures/sky/sky_back.png", 4);

    std::vector<uint8_t> textureData = GenerateTextureCubeData(g_graphicsContext, left, right, top, bottom, front, back);

    TextureCube::Descriptor skyboxDesc = {};
    skyboxDesc.width = left.Width();
    skyboxDesc.height = left.Height();
    skyboxDesc.data = textureData.data();
    skyboxDesc.format = PixelFormat::RGBA8;
    skyboxDesc.usage = UsageFlag::Static;
    skyboxDesc.bindFlags = BindFlag::ShaderResource;
    skyboxDesc.layout = TextureCube::Layout::Horizontal;
    skyboxDesc.mipLevels = GetMaxMipLevels(skyboxDesc.width, skyboxDesc.height);

    g_skybox = g_graphicsContext->CreateTextureCube(skyboxDesc);

    GraphicsShader::Descriptor skyboxShaderDesc;
    skyboxShaderDesc.vertexShaderFile = "./assets/shaders/sky.vert.spv";
    skyboxShaderDesc.vertexShaderEntry = "main";
    skyboxShaderDesc.pixelShaderFile = "./assets/shaders/sky.frag.spv";
    skyboxShaderDesc.pixelShaderEntry = "main";

    g_skyboxShader = g_graphicsContext->CreateGraphicsShader(skyboxShaderDesc);

    ShaderResourcesLayout::Descriptor skyboxResourceLayoutDesc0;
    skyboxResourceLayoutDesc0.bindings = {
        { 0, ResourceType::ConstantBuffer, ShaderCompileFlag::Vertex, 1 }
    };

    g_skyboxResourcesLayout0 = g_graphicsContext->CreateShaderResourcesLayout(skyboxResourceLayoutDesc0);

    ShaderResourcesLayout::Descriptor skyboxResourceLayoutDesc1;
    skyboxResourceLayoutDesc1.bindings = {
        { 0, ResourceType::TextureCube, ShaderCompileFlag::Pixel, 1 }
    };

    g_skyboxResourcesLayout1 = g_graphicsContext->CreateShaderResourcesLayout(skyboxResourceLayoutDesc1);

    ShaderResources::Descriptor skyboxResourcesDesc0;
    skyboxResourcesDesc0.layout = g_skyboxResourcesLayout0;

    g_skyboxResources0 = g_graphicsContext->CreateShaderResources(skyboxResourcesDesc0);
    g_skyboxResources0->BindConstantBuffer(g_cameraConstants, 0);

    ShaderResources::Descriptor skyboxResourcesDesc1;
    skyboxResourcesDesc1.layout = g_skyboxResourcesLayout1;

    g_skyboxResources1 = g_graphicsContext->CreateShaderResources(skyboxResourcesDesc1);
    g_skyboxResources1->BindTextureCube(g_skybox, 0);

    g_skyboxPipeline = g_graphicsContext->CreateGraphicsPipeline();
    g_skyboxPipeline->SetShader(g_skyboxShader);
    g_skyboxPipeline->AddShaderResourcesLayout(g_skyboxResourcesLayout0);
    g_skyboxPipeline->AddShaderResourcesLayout(g_skyboxResourcesLayout1);
    g_skyboxPipeline->SetDepthTest(DepthTest::LessEqual, false);
    g_skyboxPipeline->SetBehaviorStates(
        GraphicsPipeline::BehaviorFlag::AutoResizeViewport |
        GraphicsPipeline::BehaviorFlag::AutoResizeScissor
    );
}

void ReleaseSkyboxResources() {
    g_skybox.reset();
    g_skyboxShader.reset();
    g_skyboxResourcesLayout0.reset();
    g_skyboxResourcesLayout1.reset();
    g_skyboxResources0.reset();
    g_skyboxResources1.reset();
    g_skyboxPipeline.reset();
}