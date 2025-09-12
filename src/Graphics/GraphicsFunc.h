#pragma once

#include "Core.h"
#include "GraphicsType.h"
#include "Math/Math.h"

namespace flaw {
	inline uint32_t GetSizePerPixel(const PixelFormat format) {
		switch (format) {
		case PixelFormat::R8Unorm:
		case PixelFormat::R8_UINT:
			return 1;
		case PixelFormat::RG8:
			return 2;
		case PixelFormat::RGB8:
			return 3;
		case PixelFormat::RGBA8Unorm:
		case PixelFormat::BGRX8Unorm:
		case PixelFormat::RGBA8Srgb:
		case PixelFormat::R32F:
		case PixelFormat::R32_UINT:
		case PixelFormat::D24S8_UINT:
		case PixelFormat::D32F:
			return 4;
		case PixelFormat::D32F_S8UI:
			return 5;
		case PixelFormat::RGB16F:
			return 6;
		case PixelFormat::RGBA16F:
			return 8;
		case PixelFormat::RGB32F:
			return 12;
		case PixelFormat::RGBA32F:
			return 16;
		}

		throw std::runtime_error("Unknown pixel format");
	}

	inline bool IsDepthFormat(PixelFormat format) {
		switch (format) {
		case PixelFormat::D24S8_UINT:
		case PixelFormat::D32F_S8UI:
		case PixelFormat::D32F:
			return true;
		default:
			return false;
		}
	}

	inline uint32_t GetElementSize(const ElementType type) {
		switch (type) {
		case ElementType::Float:
			return sizeof(float);
		case ElementType::Uint32:
			return sizeof(uint32_t);
		case ElementType::Int:
			return sizeof(int32_t);
		}

		throw std::runtime_error("Unknown element type");
	}

	inline uint32_t GetMaxMipLevels(uint32_t width, uint32_t height) {
		uint32_t maxLevel = 0;
		while (width > 1 || height > 1) {
			width = std::max(1u, width / 2);
			height = std::max(1u, height / 2);
			maxLevel++;
		}
		return maxLevel;
	}

	inline void GetChangedPixelFormat(const PixelFormat srcFormat, const std::vector<uint8_t>& src, const PixelFormat dstFormat, std::vector<uint8_t>& dst) {
		if (srcFormat == dstFormat) {
			dst = src;
			return;
		}

		const uint32_t srcSizePerPixel = GetSizePerPixel(srcFormat);
		const uint32_t dstSizePerPixel = GetSizePerPixel(dstFormat);

		if (src.size() % srcSizePerPixel != 0) {
			throw std::runtime_error("Source data size is not aligned with the source pixel format.");
		}

		const uint32_t pixelCount = src.size() / srcSizePerPixel;
		const uint32_t dstSize = pixelCount * dstSizePerPixel;
		
		dst.resize(dstSize);
		if (srcFormat == PixelFormat::RGB8 && dstFormat == PixelFormat::RGBA8Unorm) {
			for (uint32_t i = 0; i < pixelCount; ++i) {
				dst[i * 4 + 0] = src[i * 3 + 0];
				dst[i * 4 + 1] = src[i * 3 + 1];
				dst[i * 4 + 2] = src[i * 3 + 2];
				dst[i * 4 + 3] = 255;
			}
		}
		else if (srcFormat == PixelFormat::RG8 && dstFormat == PixelFormat::RGBA8Unorm) {
			for (uint32_t i = 0; i < pixelCount; ++i) {
				uint8_t alpha = src[i * 2 + 1];
				uint8_t gray = alpha ? src[i * 2 + 0] : 0;

				dst[i * 4 + 0] = gray;
				dst[i * 4 + 1] = gray;
				dst[i * 4 + 2] = gray;
				dst[i * 4 + 3] = alpha;
			}
		}
		else {
			throw std::runtime_error(
				"Unsupported pixel format conversion: " +
				std::to_string(static_cast<int>(srcFormat)) + " -> " +
				std::to_string(static_cast<int>(dstFormat))
			);
		}
	}

	inline void GeneratePolygonVertices(float radius, uint32_t segments, std::vector<vec3>& outVec) {
		const float step = 360.0f / segments;
		for (uint32_t i = 0; i < segments; i++) {
			const float radian = glm::radians(i * step);
			outVec.emplace_back(radius * cosf(radian), radius * sinf(radian), 0.0f);
		}
	}

	inline void GenerateQuad(std::function<void(vec3, vec2, vec3, vec3, vec3)> vertices, std::vector<uint32_t>& outIndices, int32_t tilingX = 1, int32_t tilingY = 1, bool triIndices = true) {
		vec2 startLT = { -0.5f, 0.5f };
		vec2 startUVLT = { 0.0f, 0.0f };

		float elmentX = 1.0f / static_cast<float>(tilingX);
		float elmentY = 1.0f / static_cast<float>(tilingY);

		for (int32_t i = 0; i <= tilingY; i++) {
			for (int32_t j = 0; j <= tilingX; j++) {
				vec3 pos = { startLT.x + j * elmentX, startLT.y - i * elmentY, 0.0f };
				vec2 uv = { startUVLT.x + j * elmentX, startUVLT.y + i * elmentY };
				vec3 normal = Backward;
				vec3 tangent = Right;
				vec3 binormal = Down;

				vertices(pos, uv, normal, tangent, binormal);
			}
		}

		if (triIndices) {
			// 0, 1, 3, 3, 2, 0
			outIndices.reserve(tilingX * tilingY * 6);
			for (int32_t i = 0; i < tilingY; i++) {
				for (int32_t j = 0; j < tilingX; j++) {
					uint32_t index = i * (tilingX + 1) + j;
					outIndices.push_back(index);
					outIndices.push_back(index + 1);
					outIndices.push_back(index + (tilingX + 1) + 1);
					outIndices.push_back(index + (tilingX + 1) + 1);
					outIndices.push_back(index + (tilingX + 1));
					outIndices.push_back(index);
				}
			}
		}
		else {
			// 0, 1, 3, 2
			outIndices.reserve(tilingX * tilingY * 4);
			for (int32_t i = 0; i < tilingY; i++) {
				for (int32_t j = 0; j < tilingX; j++) {
					uint32_t index = i * (tilingX + 1) + j;
					outIndices.push_back(index);
					outIndices.push_back(index + 1);
					outIndices.push_back(index + (tilingX + 1) + 1);
					outIndices.push_back(index + (tilingX + 1));
				}
			}
		}
	}

	inline void GenerateCube(std::function<void(vec3 pos, vec2 uv, vec3 normal)> vertices, std::vector<uint32_t>& outIndices) {
		const vec3 positions[6][4] = {
			// Front (Z-)
			{ {-0.5f,  0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f} },
			// Right (X+)
			{ {0.5f,  0.5f, -0.5f}, {0.5f,  0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f, -0.5f, -0.5f} },
			// Back (Z+)
			{ {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f} },
			// Left (X-)
			{ {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f} },
			// Top (Y+)
			{ {-0.5f,  0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f} },
			// Bottom (Y-)
			{ {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f} }
		};

		const vec3 normals[6] = {
			{  0,  0, -1 }, // Front
			{  1,  0,  0 }, // Right
			{  0,  0,  1 }, // Back
			{ -1,  0,  0 }, // Left
			{  0,  1,  0 }, // Top
			{  0, -1,  0 }  // Bottom
		};

		const vec2 uvs[4] = {
			{ 0.f, 0.f },
			{ 1.f, 0.f },
			{ 1.f, 1.f },
			{ 0.f, 1.f }
		};

		uint32_t baseIndex = 0;
		for (int face = 0; face < 6; ++face)
		{
			for (int i = 0; i < 4; ++i) {
				vertices(positions[face][i], uvs[i], normals[face]);
			}

			// Index order same as GenerateCubeMesh (CW winding)
			outIndices.push_back(baseIndex + 0);
			outIndices.push_back(baseIndex + 1);
			outIndices.push_back(baseIndex + 2);
			outIndices.push_back(baseIndex + 0);
			outIndices.push_back(baseIndex + 2);
			outIndices.push_back(baseIndex + 3);

			baseIndex += 4;
		}
	}

	inline void GenerateSphere(std::function<void(vec3 pos, vec2 uv, vec3 normal)> vertices, std::vector<uint32_t>& outIndices, uint32_t sectorCount, uint32_t stackCount, float radius = 1.0f) {
		const float PI = 3.14159265359f;

		for (uint32_t i = 0; i <= stackCount; ++i)
		{
			float stackAngle = PI / 2 - i * (PI / stackCount); // +Y(�ϱ�) -> -Y(����)
			float y = radius * sinf(stackAngle);               // Y�� ����
			float r = radius * cosf(stackAngle);               // ���� ������

			for (uint32_t j = 0; j <= sectorCount; ++j)
			{
				float sectorAngle = j * (2 * PI / sectorCount); // 0 -> 360��

				float x = r * cosf(sectorAngle);
				float z = r * sinf(sectorAngle);

				vec3 position = { x, y, z };
				vec3 normal = normalize(position);

				vec2 uv = {
					(float)j / sectorCount, // u: �浵
					(float)i / stackCount   // v: ����
				};

				vertices(position, uv, normal);
			}
		}

		for (uint32_t i = 0; i < stackCount; ++i)
		{
			for (uint32_t j = 0; j < sectorCount; ++j)
			{
				uint32_t first = i * (sectorCount + 1) + j;
				uint32_t second = first + sectorCount + 1;

				// Triangle 1 (CW)
				outIndices.push_back(first);
				outIndices.push_back(first + 1);
				outIndices.push_back(second + 1);

				// Triangle 2 (CW)
				outIndices.push_back(first);
				outIndices.push_back(second + 1);
				outIndices.push_back(second);
			}
		}
	}

	inline vec3 GetTriangleNormal(vec3 p0, vec3 p1, vec3 p2) {
		vec3 edge1 = p1 - p0;
		vec3 edge2 = p2 - p0;
		return normalize(cross(edge1, edge2));
	}

	inline void GenerateTangent(const std::function<void(uint32_t index, vec3& pos, vec2& uv)>& getVertex, const std::vector<uint32_t>& indices, const std::function<void(uint32_t index, vec3& tangent)>& result) {
		for (uint32_t i = 0; i < indices.size(); i += 3) {
			uint32_t index0 = indices[i];
			uint32_t index1 = indices[i + 1];
			uint32_t index2 = indices[i + 2];

			vec3 pos0, pos1, pos2;
			vec2 uv0, uv1, uv2;
			getVertex(index0, pos0, uv0);
			getVertex(index1, pos1, uv1);
			getVertex(index2, pos2, uv2);

			vec3 edge1 = pos1 - pos0;
			vec3 edge2 = pos2 - pos0;
			vec2 deltaUV1 = uv1 - uv0;
			vec2 deltaUV2 = uv2 - uv0;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			vec3 tangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent = normalize(tangent);

			result(index0, tangent);
			result(index1, tangent);
			result(index2, tangent);
		}
	}

	inline void GenerateCone(std::function<void(vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal)> emitVertex, std::vector<uint32_t>& outIndices, uint32_t sliceCount, float radius, float height) {
		const float halfHeight = height * 0.5f;

		vec3 tipPos = vec3(0.0f, halfHeight, 0.0f);
		vec3 circleCenterPos = vec3(0.0f, -halfHeight, 0.0f);

		const uint32_t tipIndex = 0;
		emitVertex(tipPos, vec2(0.5f, 1.0f), Up, Right, cross(Up, Right)); // Tip vertex

		const uint32_t circleCenterIndex = 1;
		emitVertex(circleCenterPos, vec2(0.5f, 0.0f), -Up, -Right, cross(-Up, -Right)); // Circle center vertex

		const float radianStep = glm::two_pi<float>() / sliceCount;

		uint32_t baseStartIndex = 2;
		for (int32_t i = 0; i < sliceCount; i++) {
			const float currentRadian = i * radianStep;
			const float nextRadian = ((i + 1) % sliceCount) * radianStep;

			vec3 currentCirclePos = { radius * cosf(currentRadian), -halfHeight, radius * sinf(currentRadian) };
			vec3 nextCirclePos = { radius * cosf(nextRadian), -halfHeight, radius * sinf(nextRadian) };

			vec3 normal = GetTriangleNormal(tipPos, nextCirclePos, currentCirclePos);
			vec3 tangent = normalize(cross(normal, Right));
			vec3 binormal = normalize(cross(tangent, normal));

			emitVertex(currentCirclePos, vec2(0.0f, 0.0f), normal, tangent, binormal); // Current circle vertex
			baseStartIndex++;
			
			emitVertex(nextCirclePos, vec2(0.0f, 0.0f), normal, tangent, binormal);
			baseStartIndex++;

			outIndices.push_back(tipIndex);
			outIndices.push_back(baseStartIndex - 1);
			outIndices.push_back(baseStartIndex - 2);

			normal = -Up;
			tangent = normalize(cross(normal, Right));
			binormal = normalize(cross(tangent, normal));

			emitVertex(currentCirclePos, vec2(cosf(currentRadian) * 0.5f + 0.5f, sinf(currentRadian) * 0.5f + 0.5f), normal, tangent, binormal); // Current circle vertex
			baseStartIndex++;

			emitVertex(nextCirclePos, vec2(cosf(nextRadian) * 0.5f + 0.5f, sinf(nextRadian) * 0.5f + 0.5f), normal, tangent, binormal); // Circle center vertex
			baseStartIndex++;

			outIndices.push_back(circleCenterIndex);
			outIndices.push_back(baseStartIndex - 2);
			outIndices.push_back(baseStartIndex - 1);
		}
	}

	inline void CreateBoundingCube(const std::vector<vec3>& vertices, std::vector<vec3>& outCube) {
		vec3 min = vertices[0];
		vec3 max = vertices[0];
		for (const auto& vertex : vertices) {
			min = glm::min(min, vertex);
			max = glm::max(max, vertex);
		}

		outCube.resize(8);
		outCube[0] = vec3(min.x, max.y, min.z);
		outCube[1] = vec3(max.x, max.y, min.z);
		outCube[2] = vec3(max.x, min.y, min.z);
		outCube[3] = vec3(min.x, min.y, min.z);
		outCube[4] = vec3(min.x, max.y, max.z);
		outCube[5] = vec3(max.x, max.y, max.z);
		outCube[6] = vec3(max.x, min.y, max.z);
		outCube[7] = vec3(min.x, min.y, max.z);
	}

	inline uint32_t CalculateDispatchGroupCount(uint32_t threadCount, uint32_t targetCount) {
		return (targetCount + threadCount - 1) / threadCount;
	}

	inline uint32_t GetSamplePos(const std::vector<uint8_t>& src, uint32_t width, uint32_t height, uint32_t sizePerPixel, float u, float v) {
		const uint32_t x = std::min(static_cast<uint32_t>(u * width), width - 1);
		const uint32_t y = std::min(static_cast<uint32_t>(v * height), height - 1);

		const uint32_t index = (y * width + x) * sizePerPixel;
		if (index + sizePerPixel > src.size()) {
			throw std::runtime_error("Sample index out of bounds");
		}

		return index;
	}

	inline const uint8_t* Sample(const std::vector<uint8_t>& src, uint32_t width, uint32_t height, uint32_t sizePerPixel, float u, float v) {
		return src.data() + GetSamplePos(src, width, height, sizePerPixel, u, v);
	}
}