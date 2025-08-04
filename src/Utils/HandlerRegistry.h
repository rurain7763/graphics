#include <iostream>
#include <unordered_map>
#include <functional>
#include <atomic>

namespace flaw {
	using HandlerId = std::size_t;

	template<typename Ret, typename... Args>
	class HandlerRegistry {
	public:
		using FuncType = std::function<Ret(Args...)>;

		void Register(HandlerId id, FuncType handler) {
			_handlers.emplace(id, std::move(handler));
		}

		template<typename ClassType>
		void Register(HandlerId id, ClassType* instance, Ret(ClassType::* method)(Args...)) {
			return Register([instance, method](Args... args) -> Ret {
				return (instance->*method)(std::forward<Args>(args)...);
			});
		}

		template<typename ClassType>
		void Register(HandlerId id, ClassType* instance, Ret(ClassType::* method)(Args...) const) {
			return Register([instance, method](Args... args) -> Ret {
				return (instance->*method)(std::forward<Args>(args)...);
			});
		}

		void Unregister(HandlerId id) {
			_handlers.erase(id);
		}

		void Invoke(Args... args) {
			for (auto& [id, handler] : _handlers) {
				handler(std::forward<Args>(args)...);
			}
		}

		bool InvokeById(HandlerId id, Args... args) {
			auto it = _handlers.find(id);
			if (it != _handlers.end()) {
				it->second(std::forward<Args>(args)...);
				return true;
			}
			return false;
		}

	private:
		std::unordered_map<HandlerId, FuncType> _handlers;
	};
}
