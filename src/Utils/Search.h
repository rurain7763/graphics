#pragma once

#include "Core.h"

#include <vector>
#include <functional>

namespace flaw {
	template<typename T>
	inline int32_t BinarySearch(const std::vector<T>& data, const std::function<int32_t(const T&)>& predicate) {
		int32_t left = 0;
		int32_t right = static_cast<int32_t>(data.size()) - 1;

		while (left <= right) {
			int32_t mid = left + (right - left) / 2;
			int32_t result = predicate(data[mid]);
			if (result == 0) {
				return mid;
			}
			else if (result < 0) {
				right = mid - 1;
			}
			else {
				left = mid + 1;
			}
		}

		return -1; // Not found
	}

	template<typename T>
	inline int32_t Lowerbound(const std::vector<T>& data, const T& value) {
		int32_t left = 0;
		int32_t right = static_cast<int32_t>(data.size());

		while (left < right) {
			int32_t mid = left + (right - left) / 2;
			if (data[mid] < value) {
				left = mid + 1;
			}
			else {
				right = mid;
			}
		}

		return left;
	}

	template<typename T>
	inline int32_t Lowerbound(const std::vector<T>& data, const std::function<bool(const T&)>& predicate) {
		int32_t left = 0;
		int32_t right = static_cast<int32_t>(data.size());

		while (left < right) {
			int32_t mid = left + (right - left) / 2;
			if (predicate(data[mid])) {
				right = mid;
			}
			else {
				left = mid + 1;
			}
		}

		return left;
	}

	template<typename T>
	inline int32_t Upperbound(const std::vector<T>& data, const T& value) {
		int32_t left = 0;
		int32_t right = static_cast<int32_t>(data.size());

		while (left < right) {
			int32_t mid = left + (right - left) / 2;
			if (data[mid] <= value) {
				left = mid + 1;
			}
			else {
				right = mid;
			}
		}

		return left;
	}

	template<typename T>
	inline int32_t Upperbound(const std::vector<T>& data, const std::function<bool(const T&)>& predicate) {
		int32_t left = 0;
		int32_t right = static_cast<int32_t>(data.size());
		while (left < right) {
			int32_t mid = left + (right - left) / 2;
			if (predicate(data[mid])) {
				left = mid + 1;
			}
			else {
				right = mid;
			}
		}
		return left;
	}
}