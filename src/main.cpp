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

using namespace flaw;

int main() {
    World_Init();
    Outliner_Init();

    struct ObjectCreateInfo {
        vec3 position;
		float rotation;
		bool drawOutline;
		std::string meshKey;
    };

	ObjectCreateInfo objectCreateInfos[] = {
		{ { 0.0f, 0.0f, 0.0f }, 0, false, "sponza" },
		{ { 2.0f, 2.0f, 0.0f }, 0, true, "cube" },
		{ { -2.0f, 2.0f, 0.0f }, 0, true, "sphere" },
		{ { 0.0f, 2.0f, 0.0f }, 0, true, "cube" },
		{ { 0.0f, 4.0f, 0.0f }, 0, true, "sphere" },
	};

    for (int32_t i = 0; i < sizeof(objectCreateInfos) / sizeof(ObjectCreateInfo); i++) {
		const auto& info = objectCreateInfos[i];

 	    auto& obj = AddObject(info.meshKey.c_str());
        obj.position = info.position;
        obj.rotation = glm::vec3(info.rotation);
		obj.drawOutline = info.drawOutline;
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
            commandQueue.BeginRenderPass();

			Outliner_Render();
            World_Render();

            commandQueue.EndRenderPass();

            commandQueue.Submit();

            commandQueue.Present();
        }
    }

    Outliner_Cleanup();
    World_Cleanup();

    return 0;
}

