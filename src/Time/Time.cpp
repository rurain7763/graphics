#include "pch.h"
#include "Time.h"

#include <chrono>

namespace flaw {
	static uint64_t start;

	static float prevFrameTime;
	static uint32_t currentFrameCount;

	static uint32_t fps;
	static float deltaTime;
	static float time;
	
	uint64_t Time::GetTimeSinceEpoch() {
		auto now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	}

	void Time::Start() {
		start = GetTimeSinceEpoch();
	}

	void Time::Update() {
		uint64_t now = GetTimeSinceEpoch();
		deltaTime = (now - start) * 1e-9f;
		deltaTime = std::min(deltaTime, 0.1f); // Cap delta time to prevent large spikes

		currentFrameCount++;
		time += deltaTime;
		
		if (time - prevFrameTime >= 1.0f) {
			fps = currentFrameCount;
			currentFrameCount = 0;
			prevFrameTime = time;
		}

		start = now;
	}

	uint32_t Time::FPS() {
		return fps;
	}

	float Time::DeltaTime() {
		return deltaTime;
	}

	float Time::GetTime() {
		return time;
	}
}
