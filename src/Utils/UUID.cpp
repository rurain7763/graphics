#include "pch.h"
#include "UUID.h"

#include <random>

namespace flaw {
	static std::random_device g_rd;
	static std::mt19937_64 g_gen(g_rd());
	static std::uniform_int_distribution<uint64_t> g_dist;

	UUID::UUID() : _id(UUID_INVALID) {}
	UUID::UUID(uint64_t id) : _id(id) {}

	UUID& UUID::Generate() {
		_id = g_dist(g_gen);
		while (_id == UUID_INVALID) {
			_id = g_dist(g_gen);
		}
		return *this;
	}

	UUID& UUID::Invalidate() {
		_id = UUID_INVALID;
		return *this;
	}
}