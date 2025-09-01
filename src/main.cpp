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
    Skybox_Init();
    Outliner_Init();
    Sprite_Init();

    srand(static_cast<uint32_t>(time(0)));

    LoadTexture("assets/textures/grass.png", "grass");
    LoadTexture("assets/textures/window.png", "window");
    LoadTexture("assets/textures/haus.jpg", "haus");
    LoadTexture("assets/textures/container2.png", "container2");
    LoadTexture("assets/textures/container2_specular.png", "container2_specular");

    //LoadModel("assets/models/girl.obj", 1.0f, "girl");
    LoadModel("assets/models/survival-guitar-backpack/backpack.obj", 1.0f, "survival_backpack");
    LoadModel("assets/models/Sponza/Sponza.gltf", 0.05f, "sponza");

    struct ObjectCreateInfo {
        vec3 position;
		float rotation;
        float scale;
		bool drawOutline;
		std::string meshKey;
    };

	ObjectCreateInfo objectCreateInfos[] = {
		{ { 0.0f, -120.0f, 0.0f }, 0, 1.0 ,false, "sponza" },
		{ { 2.0f, 2.0f, 0.0f }, 0, 1.0, true, "cube" },
		{ { -2.0f, 2.0f, 0.0f }, 0, 1.0, true, "sphere" },
		{ { 0.0f, 2.0f, 0.0f }, 0, 1.0, true, "cube" },
		{ { 0.0f, 4.0f, 0.0f }, 0, 1.0, true, "sphere" },
		{ { -4.0f, 0.0f, 0.0f }, 0, 1.0, true, "survival_backpack" },
	};

    for (int32_t i = 0; i < sizeof(objectCreateInfos) / sizeof(ObjectCreateInfo); i++) {
		const auto& info = objectCreateInfos[i];

 	    auto& obj = AddObject();
        obj.position = info.position;
        obj.rotation = glm::vec3(info.rotation);
		obj.scale = glm::vec3(info.scale);

        auto meshComp = obj.AddComponent<StaticMeshComponent>();
        meshComp->mesh = GetMesh(info.meshKey.c_str());
        meshComp->drawOutline = info.drawOutline;
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

		if (g_context->GetWindowSizeState() == WindowSizeState::Minimized) {
			continue; // Skip rendering if the window is minimized
		}

        if (g_graphicsContext->Prepare()) {
			uint32_t frameIndex = commandQueue.GetCurrentFrameIndex();
			auto sceneFramebuffer = g_sceneFramebuffers[frameIndex];

            commandQueue.BeginRenderPass(g_sceneClearRenderPass, g_sceneLoadRenderPass, sceneFramebuffer);
			Outliner_Render();
            World_Render();
            Skybox_Render();
            Sprite_Render();
            commandQueue.EndRenderPass();

            auto attachment = std::static_pointer_cast<Texture2D>(sceneFramebuffer->GetColorAttachment(0));

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

    Sprite_Cleanup();
    Outliner_Cleanup();
	Skybox_Cleanup();
    Asset_Cleanup();
    World_Cleanup();

    return 0;
}

