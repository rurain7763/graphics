#include "pch.h"
#include "EventDispatcher.h"

namespace flaw {
	EventDispatcher::~EventDispatcher() {
		_eventRegistries.clear();
	}

	void EventDispatcher::UnregisterAll(uint64_t listenerID) {
		std::vector<std::type_index> toRemove;
		for (const auto& [typeIndex, registry] : _eventRegistries) {
			registry->Unregister(listenerID);

			if (!registry->HasListeners()) {
				toRemove.push_back(typeIndex);
			}
		}

		for (const auto& typeIndex : toRemove) {
			_eventRegistries.erase(typeIndex);
		}
	}

	void EventDispatcher::PollEvents() {
		for (const auto& [_, registry] : _eventRegistries) {
			registry->PollEvents();
		}
	}
}