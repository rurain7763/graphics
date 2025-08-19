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

    AddObject("girl");

    auto& commandQueue = g_graphicsContext->GetCommandQueue();
    
    Time::Start();
    while (g_context->PollEvents()) {
        g_eventDispatcher.PollEvents();

        Time::Update();

        std::string title = "Flaw Application - FPS: " + std::to_string(Time::FPS()) + " | Delta Time: " + std::to_string(Time::DeltaTime() * 1000.0f) + " ms";
        g_context->SetTitle(title.c_str());

        World_Update();

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

