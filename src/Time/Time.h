#pragma once

#include "Core.h"

namespace flaw {
	class FAPI Time {
	public:
		static void Start();
		static void Update();
		
		static uint32_t FPS();
		static float DeltaTime();
		static float GetTime();
		
	private:
		static uint64_t GetTimeSinceEpoch();
	};
}