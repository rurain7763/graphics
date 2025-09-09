#include "pch.h"
#include "Log/Log.h"
#include "Event/EventDispatcher.h"
#include "Platform/PlatformContext.h"
#include "Platform/PlatformEvents.h"
#include "Graphics/Vulkan/VkContext.h"
#include "Graphics/DX11/DXContext.h"
#include "Time/Time.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "Model/Model.h"
#include "world.h"
#include "outliner.h"
#include "sprite.h"
#include "asset.h"

using namespace flaw;

int main() {
    World_Init();
    Asset_Init();

    srand(static_cast<uint32_t>(time(0)));

    LoadTexture("assets/textures/grass.png", PixelFormat::RGBA8Srgb, "grass");
    LoadTexture("assets/textures/window.png", PixelFormat::RGBA8Srgb, "window");
    LoadTexture("assets/textures/haus.jpg", PixelFormat::RGBA8Srgb, "haus");
    LoadTexture("assets/textures/container2.png", PixelFormat::RGBA8Srgb, "container2");
    LoadTexture("assets/textures/container2_specular.png", PixelFormat::RGBA8Srgb, "container2_specular");
	LoadTexture("assets/textures/container2_norm.png", PixelFormat::RGBA8Unorm, "container2_norm");
	LoadTexture("assets/textures/brickwall.jpg", PixelFormat::RGBA8Srgb, "brickwall");
	LoadTexture("assets/textures/brickwall_normal.jpg", PixelFormat::RGBA8Unorm, "brickwall_normal");
	LoadTexture("assets/textures/brickwall_disp.jpg", PixelFormat::RGBA8Unorm, "brickwall_disp");
	LoadTexture("assets/textures/paving.png", PixelFormat::RGBA8Srgb, "paving");
	LoadTexture("assets/textures/paving_spec.png", PixelFormat::RGBA8Unorm, "paving_spec");
	LoadTexture("assets/textures/paving_norm.png", PixelFormat::RGBA8Unorm, "paving_norm");

    LoadModel("assets/models/girl.obj", 1.0f, "girl");
    LoadModel("assets/models/survival-guitar-backpack/backpack.obj", 1.0f, "survival_backpack");
    LoadModel("assets/models/Sponza/Sponza.gltf", 0.05f, "sponza");
    LoadModel("assets/models/planet/planet.obj", 1.0f, "planet");
	LoadModel("assets/models/rock/rock.obj", 1.0f, "rock");

	std::vector<TexturedVertex> sphereVertices;
	std::vector<uint32_t> sphereIndices;

    GenerateSphere(
        [&](const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal) {
            TexturedVertex vertex;
            vertex.position = position;
            vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            vertex.texCoord = texCoord;
            vertex.normal = normal;
            sphereVertices.push_back(vertex);
        },
        sphereIndices,
        32,
        32,
        0.5f
    );

    GenerateTangent(
        [&](uint32_t index, glm::vec3& pos, glm::vec2& uv) {
            pos = sphereVertices[index].position;
            uv = sphereVertices[index].texCoord;
        },
        sphereIndices,
        [&](uint32_t index, const glm::vec3& tangent) {
            sphereVertices[index].tangent = tangent;
        }
    );

	LoadPrimitiveModel(sphereVertices, sphereIndices, "brickwall");
    LoadMaterial("brickwall");

	auto container2Material = GetMaterial("brickwall");
	container2Material->diffuseTexture = GetTexture2D("brickwall");
	container2Material->normalTexture = GetTexture2D("brickwall_normal");
	container2Material->displacementTexture = GetTexture2D("brickwall_disp");

	auto containerMesh = GetMesh("brickwall");
	containerMesh->materials[0] = container2Material;

    container2Material = nullptr;
	containerMesh = nullptr;

	LoadPrimitiveModel(sphereVertices, sphereIndices, "paving");
	LoadMaterial("paving");

	auto pavingMaterial = GetMaterial("paving");
	pavingMaterial->diffuseTexture = GetTexture2D("paving");
	pavingMaterial->specularTexture = GetTexture2D("paving_spec");
	pavingMaterial->normalTexture = GetTexture2D("paving_norm");
	pavingMaterial->diffuseColor = vec3(1.0f);
	pavingMaterial->specularColor = vec3(1.0f);
	pavingMaterial->shininess = 32.0f;

	auto pavingMesh = GetMesh("paving");
	pavingMesh->materials[0] = pavingMaterial;

	pavingMaterial = nullptr;
	pavingMesh = nullptr;

    Shadow_Init();
    Skybox_Init();
    Outliner_Init();
    Sprite_Init();
#if USE_VULKAN
    Geometry_Init();
#endif

	g_camera->SetPosition({ 0.0f, 0.0f, -8.0f });

    struct ObjectCreateInfo {
        std::string name;
        vec3 position;
		vec3 rotation;
        vec3 scale;
		bool drawOutline;
		bool drawNormal;
		std::string meshKey;
    };

    std::vector<ObjectCreateInfo> objectCreateInfos = {
        { "", { 0.0f, -120.0f, 0.0f }, vec3(0), vec3(1.0), false, false, "sponza" },
        { "", { -4.0f, 0.0f, 0.0f }, vec3(0), vec3(1.0), true, true, "sphere" },
        { "paving", vec3(4.0f, 0.0f, 0.0f), vec3(0), vec3(1.0), false, true, "paving" },
        { "brickwall", vec3(4.0f, 4.0f, 0.0f), vec3(0), vec3(1.0), false, true, "brickwall" },
        { "", { 0.0f, 0.0f, 4.0f }, vec3(0), vec3(1.0), true, false, "cube" },
        { "", { 0.0f, 0.0f, -4.0f }, vec3(0), vec3(1.0), true, false, "cube" },
        { "", { 0.0f, 4.0f, 0.0f }, vec3(0), vec3(1.0), true, false, "sphere" },
        { "", vec3(0.0f, 0.0f, 10.0f), vec3(glm::half_pi<float>(), 0, 0), vec3(20.0, 0.5, 20.0), false, false, "cube" },
        { "", vec3(10.0f, 0.0f, 0.0f), vec3(0, 0, glm::half_pi<float>()), vec3(20.0, 0.5, 20.0), false, false, "cube" },
        { "", vec3(-10.0f, 0.0f, 0.0f), vec3(0, 0, glm::half_pi<float>()), vec3(20.0, 0.5, 20.0), false, false, "cube" },
        { "", vec3(0.0f, 10.0f, 0.0f), vec3(0, 0, 0), vec3(20.0, 0.5, 20.0), false, false, "cube" },
        { "", vec3(0.0f, -10.0f, 0.0f), vec3(0, 0, 0), vec3(20.0, 0.5, 20.0), false, false, "cube" },
		{ "", { 0.0f, -4.0f, 0.0f }, vec3(0, glm::pi<float>() / 3.0, 0), vec3(1.0), true, true, "survival_backpack" },
		{ "", { 20.0f, 0.0f, 0.0f }, vec3(0), vec3(1.0), false, false, "planet" },
	};

    const uint32_t amount = 1;
    float radius = 50.0;
    float offset = 2.5f;
    for (uint32_t i = 0; i < amount; i++) {
		ObjectCreateInfo info;

        glm::mat4 model = glm::mat4(1.0f);

        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;

		info.position = vec3(x, y, z) + vec3(20.f, 0, 0);
		info.scale = vec3((rand() % 20) / 100.0f + 0.05);
		info.rotation = vec3(glm::radians((float)(rand() % 360)));
        info.meshKey = "rock";
		info.drawNormal = false;
		info.drawOutline = false;

		objectCreateInfos.push_back(info);
    }

    for (int32_t i = 0; i < objectCreateInfos.size(); i++) {
		const auto& info = objectCreateInfos[i];

 	    auto& obj = AddObject();
		obj.name = info.name;
        obj.position = info.position;
        obj.rotation = glm::vec3(info.rotation);
		obj.scale = glm::vec3(info.scale);

        auto meshComp = obj.AddComponent<StaticMeshComponent>();
        meshComp->mesh = GetMesh(info.meshKey.c_str());
        meshComp->drawOutline = info.drawOutline;
		meshComp->drawNormal = info.drawNormal;
    }

    struct SpriteCreateInfo {
        vec3 position;
        std::string textureKey;
    };

    std::vector<SpriteCreateInfo> spriteCreateInfos = {
        { { rand() % 10, 0.5f, rand() % 10 }, "window" },
        { { rand() % 10, 0.5f, rand() % 10 }, "window" },
        { { rand() % 10, 0.5f, rand() % 10 }, "window" },
        { { rand() % 10, 0.5f, rand() % 10 }, "window" },
        { { rand() % 10, 0.5f, rand() % 10 }, "window" },
        { { rand() % 10, 0.5f, rand() % 10 }, "window" },
        { { rand() % 10, 0.5f, rand() % 10 }, "window" },
        { { 2.0f, 0.5f, 0.0f }, "grass" }
    };

    for (const auto& info : spriteCreateInfos) {
        auto& sprite = AddObject();
        sprite.position = info.position;

        auto spriteComp = sprite.AddComponent<SpriteComponent>();
        spriteComp->texture = GetTexture2D(info.textureKey.c_str());
    }

    auto& commandQueue = g_graphicsContext->GetCommandQueue();
    
    Time::Start();
    while (g_context->PollEvents()) {
        g_eventDispatcher.PollEvents();

        Time::Update();

        std::string title = "Flaw Application - FPS: " + std::to_string(Time::FPS()) + " | Delta Time: " + std::to_string(Time::DeltaTime() * 1000.0f) + " ms";
        g_context->SetTitle(title.c_str());

        g_camera->OnUpdate();

		World_Update();
        Shadow_Update();

		if (g_context->GetWindowSizeState() == WindowSizeState::Minimized) {
			continue; // Skip rendering if the window is minimized
		}

        if (g_graphicsContext->Prepare()) {
			auto sceneFramebuffer = g_sceneFramebufferGroup->Get();

            Shadow_Render();
            
            commandQueue.BeginRenderPass(g_sceneRenderPass, sceneFramebuffer);
            World_Render();
#if USE_VULKAN
			//Geometry_Render();
#endif
            Outliner_Render();
            Skybox_Render();
            //Sprite_Render();

			commandQueue.NextSubpass();

            World_FinalizeRender();

            commandQueue.EndRenderPass();

            commandQueue.Submit();

            commandQueue.Present();
        }
    }

#if USE_VULKAN
    Geometry_Cleanup();
#endif
    Sprite_Cleanup();
    Outliner_Cleanup();
	Skybox_Cleanup();
	Shadow_Cleanup();
    Asset_Cleanup();
    World_Cleanup();

    return 0;
}

