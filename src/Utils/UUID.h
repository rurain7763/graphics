#pragma once

#include "Core.h"

#include <xhash> 

namespace flaw {
	constexpr uint64_t UUID_INVALID = 0xFFFFFFFFFFFFFFFF;

	class UUID {
	public:
		UUID();
		UUID(uint64_t id);
		UUID(const UUID& other) = default;

		UUID& Generate();
		UUID& Invalidate();

		bool IsValid() const { return _id != UUID_INVALID; }

		UUID& operator=(const UUID& other) {
			if (this != &other) {
				_id = other._id;
			}
			return *this;
		}

		operator uint64_t() const { return _id; }

		UUID operator+(uint64_t value) const {
			return UUID(_id + value);
		}

		UUID operator-(uint64_t value) const {
			return UUID(_id - value);
		}

		UUID& operator+=(uint64_t value) {
			_id += value;
			return *this;
		}

		UUID& operator-=(uint64_t value) {
			_id -= value;
			return *this;
		}

		UUID operator++(int) {
			UUID temp = *this;
			_id++;
			return temp;
		}

		UUID& operator++() {
			_id++;
			return *this;
		}

		bool operator==(const UUID& other) const { return _id == other._id; }
		bool operator!=(const UUID& other) const { return _id != other._id; }
		bool operator<(const UUID& other) const { return _id < other._id; }

	private:
		friend struct std::hash<UUID>;

		uint64_t _id;
	};
}

namespace std {
	template<>
	struct hash<flaw::UUID> {
		size_t operator()(const flaw::UUID& uuid) const {
			return uuid._id;
		}
	};
}