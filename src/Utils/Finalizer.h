#pragma once

#include <functional>

namespace flaw {
	class Finalizer {
	public:
		Finalizer(const std::function<void()>& finalizer) : _finalizer(finalizer) {}
		~Finalizer() { _finalizer(); }

	private:
		std::function<void()> _finalizer;
	};
}