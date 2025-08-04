#pragma once

#include "Core.h"

#include <vector>
#include <unordered_map>

namespace flaw {
	template<typename Key, typename Value>
	class BiMap {
	public:
		BiMap() = default;

		void Insert(const Key& key, const Value& value) {
			if (ContainsKey(key)) {
				RemoveByKey(key);
			}

			if (ContainsValue(value)) {
				RemoveByValue(value);
			}

			_keyValueMap[key] = value;
			_valueKeyMap[value] = key;
		}

		bool ContainsKey(const Key& key) const {
			return _keyValueMap.find(key) != _keyValueMap.end();
		}

		bool ContainsValue(const Value& value) const {
			return _valueKeyMap.find(value) != _valueKeyMap.end();
		}

		const Value& GetValue(const Key& key) const {
			auto it = _keyValueMap.find(key);
			if (it != _keyValueMap.end()) {
				return it->second;
			}

			throw std::out_of_range("Key not found");
		}

		void SetValue(const Key& key, const Value& value) {
			if (ContainsKey(key)) {
				RemoveByKey(key);
			}

			if (ContainsValue(value)) {
				RemoveByValue(value);
			}

			_keyValueMap[key] = value;
			_valueKeyMap[value] = key;
		}

		const Key& GetKey(const Value& value) const {
			auto it = _valueKeyMap.find(value);
			if (it != _valueKeyMap.end()) {
				return it->second;
			}

			throw std::out_of_range("Value not found");
		}

		void SetKey(const Value& value, const Key& key) {
			if (ContainsValue(value)) {
				RemoveByValue(value);
			}

			if (ContainsKey(key)) {
				RemoveByKey(key);
			}

			_keyValueMap[key] = value;
			_valueKeyMap[value] = key;
		}

		void RemoveByKey(const Key& key) {
			auto it = _keyValueMap.find(key);
			if (it == _keyValueMap.end()) {
				return;
			}

			Value value = it->second;
			_keyValueMap.erase(it);
			_valueKeyMap.erase(value);
		}

		void RemoveByValue(const Value& value) {
			auto it = _valueKeyMap.find(value);
			if (it == _valueKeyMap.end()) {
				return;
			}

			Key key = it->second;
			_valueKeyMap.erase(it);
			_keyValueMap.erase(key);
		}

		void Clear() {
			_keyValueMap.clear();
			_valueKeyMap.clear();
		}

		size_t Size() const {
			return _keyValueMap.size();
		}

		const std::unordered_map<Key, Value>& GetKeyValueMap() const {
			return _keyValueMap;
		}

		const std::unordered_map<Value, Key>& GetValueKeyMap() const {
			return _valueKeyMap;
		}

	private:
		std::unordered_map<Key, Value> _keyValueMap;
		std::unordered_map<Value, Key> _valueKeyMap;
	};
}