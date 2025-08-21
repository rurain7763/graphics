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

#include <filesystem>

using namespace flaw;

int main() {
    World_Init();

    vec3 positions[] = {
        { 0.0f, 0.0f, 0.0f },
	    //{ 2.0f, 0.0f, 0.0f },
	    //{ -2.0f, 0.0f, 0.0f },
		//{ 0.0f, 2.0f, 0.0f },
		//{ 0.0f, -2.0f, 0.0f },
		//{ 1.5f, 1.5f, 0.0f },
		//{ -1.5f, -1.5f, 0.0f },
		//{ 1.5f, -1.5f, 0.0f },
		//{ -1.5f, 1.5f, 0.0f },
    };

    for (int32_t i = 0; i < sizeof(positions) / sizeof(vec3); i++) {
 	    auto& obj = AddObject();
		obj.position = positions[i];

		float angle = glm::radians(20.0f * i);
        obj.rotation = glm::vec3(angle);
    }

    auto& commandQueue = g_graphicsContext->GetCommandQueue();
    
    Time::Start();
    while (g_context->PollEvents()) {
        g_eventDispatcher.PollEvents();

        Time::Update();

        std::string title = "Flaw Application - FPS: " + std::to_string(Time::FPS()) + " | Delta Time: " + std::to_string(Time::DeltaTime() * 1000.0f) + " ms";
        g_context->SetTitle(title.c_str());

        g_camera->OnUpdate();

		if (g_context->GetWindowSizeState() == WindowSizeState::Minimized) {
			continue; // Skip rendering if the window is minimized
		}

        if (g_graphicsContext->Prepare()) {
            commandQueue.BeginRenderPass();

            World_Render();

            commandQueue.EndRenderPass();

            commandQueue.Submit();

            commandQueue.Present();
        }
    }

    World_Cleanup();

    return 0;
}

