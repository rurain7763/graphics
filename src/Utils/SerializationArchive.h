#pragma once

#include "Core.h"

#include <vector>
#include <unordered_map>

namespace flaw {
	class SerializationArchive;

	template <typename T>
	struct Serializer {
		static void Serialize(SerializationArchive& archive, const T& value) {}
		static void Deserialize(SerializationArchive& archive, T& value) {}
	};

	class SerializationArchive {
	public:
		SerializationArchive() = default;
		SerializationArchive(const int8_t* data, const uint64_t size) {
			_buffer.resize(size);
			std::memcpy(_buffer.data(), data, size);
		}

		// 기본 타입 직렬화
		template<typename T>
		typename std::enable_if<std::is_arithmetic_v<T>, SerializationArchive&>::type operator<<(const T& value) {
			uint32_t oldSize = _buffer.size();
			_buffer.resize(oldSize + sizeof(T));
			std::memcpy(_buffer.data() + oldSize, &value, sizeof(T));
			return *this;
		}

		// 기본 타입 역직렬화
		template<typename T>
		typename std::enable_if<std::is_arithmetic_v<T>, SerializationArchive&>::type operator>>(T& value) {
			std::memcpy(&value, _buffer.data() + _offset, sizeof(T));
			_offset += sizeof(T);
			return *this;
		}

		// 사용자 정의 타입 직렬화 (std::string 제외)
		template <typename T>
		typename std::enable_if<
			!std::is_arithmetic_v<T> &&
			!std::is_same_v<T, std::string> &&
			!std::is_enum_v<T>,
			SerializationArchive&>::type operator<<(const T& value) 
		{
			Serializer<T>::Serialize(*this, value);
			return *this;
		}

		// 사용자 정의 타입 역직렬화 (std::string 제외)
		template <typename T>
		typename std::enable_if<
			!std::is_arithmetic_v<T> &&
			!std::is_same_v<T, std::string> &&
			!std::is_enum_v<T>,
			SerializationArchive&>::type operator>>(T& value) 
		{
			Serializer<T>::Deserialize(*this, value);
			return *this;
		}

		// 열거형 직렬화
		template <typename T>
		typename std::enable_if<std::is_enum_v<T>, SerializationArchive&>::type operator<<(const T& value) {
			*this << static_cast<uint32_t>(value);
			return *this;
		}

		// 열거형 역직렬화
		template <typename T>
		typename std::enable_if<std::is_enum_v<T>, SerializationArchive&>::type operator>>(T& value) {
			uint32_t intValue;
			*this >> intValue;
			value = static_cast<T>(intValue);
			return *this;
		}

		SerializationArchive& operator<<(const std::string& value) {
			uint32_t size = value.size();
			*this << size;
			uint32_t oldSize = _buffer.size();
			_buffer.resize(oldSize + size);
			std::memcpy(_buffer.data() + oldSize, value.data(), size);
			return *this;
		}

		SerializationArchive& operator>>(std::string& value) {
			uint32_t size;
			*this >> size;
			value = std::string((char*)_buffer.data() + _offset, size);
			_offset += size;
			return *this;
		}

		template <typename T>
		SerializationArchive& operator<<(const std::vector<T>& value) {
			uint32_t size = value.size();
			*this << size;

			if constexpr (std::is_arithmetic_v<T>) {
				uint32_t oldSize = _buffer.size();
				_buffer.resize(oldSize + sizeof(T) * size);
				std::memcpy(_buffer.data() + oldSize, value.data(), sizeof(T) * size);
			}
			else {
				for (const auto& v : value) {
					*this << v;
				}
			}

			return *this;
		}

		template <typename T>
		SerializationArchive& operator>>(std::vector<T>& value) {
			uint32_t size;
			*this >> size;
			value.resize(size);

			if constexpr (std::is_arithmetic_v<T>) {
				std::memcpy(value.data(), _buffer.data() + _offset, sizeof(T) * size);
				_offset += sizeof(T) * size;
			}
			else {
				for (auto& v : value) {
					*this >> v;
				}
			}

			return *this;
		}

		template <typename TKey, typename TValue>
		SerializationArchive& operator<<(const std::unordered_map<TKey, TValue>& value) {
			uint32_t size = value.size();
			*this << size;
			for (const auto& [key, val] : value) {
				*this << key << val;
			}

			return *this;
		}

		template <typename TKey, typename TValue>
		SerializationArchive& operator>>(std::unordered_map<TKey, TValue>& value) {
			if (RemainingSize() < sizeof(uint32_t)) {
				throw std::runtime_error("Buffer underflow");
			}

			uint32_t size;
			*this >> size;
			for (uint32_t i = 0; i < size; i++) {
				TKey key;
				TValue val;
				*this >> key >> val;
				value[key] = val;
			}

			return *this;
		}

		void Consume(uint32_t size) { _offset += size; }

		void Append(const int8_t* buffer, uint32_t size) {
			_buffer.insert(_buffer.end(), buffer, buffer + size);
		}

		void Clear() {
			_buffer.clear();
			_offset = 0;
		}

		const int8_t* Data() const { return (const int8_t*)_buffer.data(); }
		uint32_t Offset() const { return _offset; }
		uint32_t RemainingSize() const { return _buffer.size() - _offset; }

	private:
		std::vector<int8_t> _buffer;
		uint32_t _offset = 0;
	};
}