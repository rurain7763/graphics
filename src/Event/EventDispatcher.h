#pragma once

#include "Core.h"

#include <queue>
#include <unordered_map>
#include <memory>
#include <functional>
#include <typeindex>
#include <stdint.h>

namespace flaw {
    struct FAPI IEventRegistry {
        virtual ~IEventRegistry() = default;
        virtual void Unregister(uint64_t listenerID) = 0;
        virtual void PollEvents() = 0;
		virtual bool HasListeners() = 0;
    };

    template <typename T>
    struct EventRegistry : public IEventRegistry {
        using CallbackFn = std::function<void(const T&)>;

        std::queue<T> eventQueue;
        std::queue<std::pair<uint64_t, T>> eventQueueToListener;
        std::unordered_map<uint64_t, CallbackFn> listeners;

        void PollEvents() override {
            while (!eventQueue.empty()) {
                for (auto& [_, listener] : listeners) {
                    listener(eventQueue.front());
                }
                eventQueue.pop();
            }

            while (!eventQueueToListener.empty()) {
                auto& [listenerID, event] = eventQueueToListener.front();
                auto listener = listeners.find(listenerID);
                if (listener != listeners.end()) {
                    listener->second(event);
                }
                eventQueueToListener.pop();
            }
        }

        void Unregister(uint64_t listenerID) override {
            listeners.erase(listenerID);
        }

        bool HasListeners() override {
            return !listeners.empty();
        }
    };

    class EventDispatcher {
    public:
        ~EventDispatcher();

        template<typename TEvent, typename TCallback>
        void Register(TCallback&& callback, uint64_t listenerID) {
            GetRegistry<TEvent>()->listeners.emplace(listenerID, std::move(callback));
        }

        template<typename TEvent>
        void Unregister(uint64_t listenerID) {
			auto* registry = GetRegistry<TEvent>();

			registry->Unregister(listenerID);
			if (!registry->HasListeners()) {
				_eventRegistries.erase(typeid(TEvent));
			}
        }

        void UnregisterAll(uint64_t listenerID);

        template<typename TEvent, typename... TArgs>
        void Dispatch(TArgs&&... args) {
            auto registry = GetRegistry<TEvent>();
            if (registry->listeners.empty()) {
                return;
            }
            registry->eventQueue.push(TEvent(std::forward<TArgs>(args)...));
        }

        template<typename TEvent, typename... TArgs>
        void DispatchToListener(uint64_t listenerID, TArgs&&... args) {
            auto registry = GetRegistry<TEvent>();
            if (registry->listeners.empty()) {
                return;
            }
            registry->eventQueueToListener.push(std::make_pair(listenerID, TEvent(std::forward<TArgs>(args)...)));
        }

        void PollEvents();

    private:
        template<typename TEvent>
        EventRegistry<TEvent>* GetRegistry() {
            const std::type_index typeID = typeid(TEvent);

            if (_eventRegistries.find(typeID) == _eventRegistries.end()) {
                _eventRegistries[typeID] = std::make_unique<EventRegistry<TEvent>>();
            }

            return static_cast<EventRegistry<TEvent>*>(_eventRegistries[typeID].get());
        }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<IEventRegistry>> _eventRegistries;
    };
}
