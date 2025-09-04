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

    LoadTexture("assets/textures/grass.png", "grass");
    LoadTexture("assets/textures/window.png", "window");
    LoadTexture("assets/textures/haus.jpg", "haus");
    LoadTexture("assets/textures/container2.png", "container2");
    LoadTexture("assets/textures/container2_specular.png", "container2_specular");

    LoadModel("assets/models/girl.obj", 1.0f, "girl");
    LoadModel("assets/models/survival-guitar-backpack/backpack.obj", 1.0f, "survival_backpack");
    LoadModel("assets/models/Sponza/Sponza.gltf", 0.05f, "sponza");
    LoadModel("assets/models/planet/planet.obj", 1.0f, "planet");
	LoadModel("assets/models/rock/rock.obj", 1.0f, "rock");

    Shadow_Init();
    Skybox_Init();
    Outliner_Init();
    Sprite_Init();
    //Geometry_Init();

    struct ObjectCreateInfo {
        vec3 position;
		float rotation;
        float scale;
		bool drawOutline;
		bool drawNormal;
		std::string meshKey;
    };

	std::vector<ObjectCreateInfo> objectCreateInfos = {
		{ { 0.0f, -120.0f, 0.0f }, 0, 1.0, false, false, "sponza" },
		{ { 2.0f, 2.0f, 0.0f }, 0, 1.0, true, true, "cube" },
		{ { -2.0f, 2.0f, 0.0f }, 0, 1.0, true, true, "sphere" },
		{ { 0.0f, 2.0f, 0.0f }, 0, 1.0, true, false, "cube" },
		{ { 0.0f, 4.0f, 0.0f }, 0, 1.0, true, false, "sphere" },
		{ { -4.0f, 0.0f, 0.0f }, 0, 1.0, true, true, "survival_backpack" },
		{ { 20.0f, 0.0f, 0.0f }, 0, 4.0, false, false, "planet" },
	};

    const uint32_t amount = 5000;
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
		info.scale = (rand() % 20) / 100.0f + 0.05;
		info.rotation = glm::radians((float)(rand() % 360));
        info.meshKey = "rock";
		info.drawNormal = false;
		info.drawOutline = false;

		objectCreateInfos.push_back(info);
    }

    for (int32_t i = 0; i < objectCreateInfos.size(); i++) {
		const auto& info = objectCreateInfos[i];

 	    auto& obj = AddObject();
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
			uint32_t frameIndex = commandQueue.GetCurrentFrameIndex();
			auto sceneFramebuffer = g_sceneFramebuffers[frameIndex];

            Shadow_Render();

            commandQueue.BeginRenderPass(g_sceneClearRenderPass, g_sceneLoadRenderPass, sceneFramebuffer);
			Outliner_Render();
            World_Render();
			//Geometry_Render();
            Skybox_Render();
            Sprite_Render();
            commandQueue.EndRenderPass();

            auto attachment = std::static_pointer_cast<Texture2D>(sceneFramebuffer->GetResolveAttachment(0));
            if (!attachment) {
				attachment = std::static_pointer_cast<Texture2D>(sceneFramebuffer->GetColorAttachment(0));
            }

            commandQueue.SetPipelineBarrier(
                attachment,
                TextureLayout::ColorAttachment,
                TextureLayout::ShaderReadOnly,
                AccessType::ColorAttachmentWrite,
                AccessType::ShaderRead,
                PipelineStage::ColorAttachmentOutput,
                PipelineStage::PixelShader
            );
            
            commandQueue.BeginRenderPass();
            World_FinalizeRender();
            commandQueue.EndRenderPass();

            commandQueue.Submit();

            commandQueue.Present();
        }
    }

    //Geometry_Cleanup();
    Sprite_Cleanup();
    Outliner_Cleanup();
	Skybox_Cleanup();
	Shadow_Cleanup();
    Asset_Cleanup();
    World_Cleanup();

    return 0;
}

