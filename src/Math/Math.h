
#pragma once

#include "Utils/SerializationArchive.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using namespace glm;

namespace flaw {
	constexpr vec3 Forward = vec3(0.0f, 0.0f, 1.0f);
	constexpr vec3 Backward = vec3(0.0f, 0.0f, -1.0f);
	constexpr vec3 Right = vec3(1.0f, 0.0f, 0.0f);
	constexpr vec3 Left = vec3(-1.0f, 0.0f, 0.0f);
	constexpr vec3 Up = vec3(0.0f, 1.0f, 0.0f);
	constexpr vec3 Down = vec3(0.0f, -1.0f, 0.0f);

	struct MathParams {
		inline static bool InvertYAxis = false;
	};

	template<>
	struct Serializer<vec2> {
		static void Serialize(SerializationArchive& archive, const vec2& value) {
			archive << value.x;
			archive << value.y;
		}

		static void Deserialize(SerializationArchive& archive, vec2& value) {
			archive >> value.x;
			archive >> value.y;
		}
	};

	template<>
	struct Serializer<vec3> {
		static void Serialize(SerializationArchive& archive, const vec3& value) {
			archive << value.x;
			archive << value.y;
			archive << value.z;
		}

		static void Deserialize(SerializationArchive& archive, vec3& value) {
			archive >> value.x;
			archive >> value.y;
			archive >> value.z;
		}
	};

	template<>
	struct Serializer<vec4> {
		static void Serialize(SerializationArchive& archive, const vec4& value) {
			archive << value.x;
			archive << value.y;
			archive << value.z;
			archive << value.w;
		}

		static void Deserialize(SerializationArchive& archive, vec4& value) {
			archive >> value.x;
			archive >> value.y;
			archive >> value.z;
			archive >> value.w;
		}
	};

	template<>
	struct Serializer<mat4> {
		static void Serialize(SerializationArchive& archive, const mat4& value) {
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					archive << value[i][j];
				}
			}
		}

		static void Deserialize(SerializationArchive& archive, mat4& value) {
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					archive >> value[i][j];
				}
			}
		}
	};

	inline bool EpsilonEqual(float a, float b, float epsilon = 1e-6f) {
		return abs(a - b) < epsilon;
	}

	inline float GetFovX(float fovY, float aspectRatio) {
		return 2.0f * atan(tan(fovY * 0.5f) * aspectRatio);
	}

	inline vec3 ExtractPosition(const mat4& matrix) {
		return vec3(matrix[3][0], matrix[3][1], matrix[3][2]);
	}

	inline vec3 ExtractScale(const mat4& matrix) {
		return vec3(length(vec3(matrix[0][0], matrix[0][1], matrix[0][2])),
					length(vec3(matrix[1][0], matrix[1][1], matrix[1][2])),
					length(vec3(matrix[2][0], matrix[2][1], matrix[2][2])));
	}

	inline vec3 ExtractRotation(const mat4& matrix) {
		float scaleX = length(vec3(matrix[0][0], matrix[0][1], matrix[0][2]));
		float scaleY = length(vec3(matrix[1][0], matrix[1][1], matrix[1][2]));
		float scaleZ = length(vec3(matrix[2][0], matrix[2][1], matrix[2][2]));

		mat4 rotationMatrix = matrix;
		rotationMatrix[0] /= scaleX;
		rotationMatrix[1] /= scaleY;
		rotationMatrix[2] /= scaleZ;

		return eulerAngles(toQuat(rotationMatrix));
	}

	inline mat4 Translate(const vec3& translation) {
		return translate(mat4(1.0f), translation);
	}

	inline mat4 QRotate(const vec3& rotation) {
		return toMat4(glm::quat(rotation));
	}

	inline vec3 QRotate(const vec3& rotation, const vec3& axis) {
		return rotate(glm::quat(rotation), axis);
	}

	inline mat4 Scale(const vec3& scale) {
		return glm::scale(mat4(1.0f), scale);
	}

	inline mat4 ModelMatrix(const vec3& position, const vec3& rotation, const vec3& scale) {
		return Translate(position) * QRotate(rotation) * Scale(scale);
	}

	inline mat4 ModelMatrix(const vec3& position, const quat& rotation, const vec3& scale) {
		return Translate(position) * toMat4(rotation) * Scale(scale);
	}

	inline void ExtractModelMatrix(const mat4& matrix, vec3& outPosition, vec3& outRotation, vec3& outScale) {
		// ��ġ ����
		outPosition.x = matrix[3][0];
		outPosition.y = matrix[3][1];
		outPosition.z = matrix[3][2];

		// ������ ����
		outScale.x = length(vec3(matrix[0][0], matrix[0][1], matrix[0][2]));
		outScale.y = length(vec3(matrix[1][0], matrix[1][1], matrix[1][2]));
		outScale.z = length(vec3(matrix[2][0], matrix[2][1], matrix[2][2]));

		// ������ ���� �� ȸ�� ����
		mat4 rotationMatrix = matrix;
		rotationMatrix[0] /= outScale.x;
		rotationMatrix[1] /= outScale.y;
		rotationMatrix[2] /= outScale.z;

		outRotation = eulerAngles(toQuat(rotationMatrix));
	}

	inline void ExtractModelMatrix(const mat4& matrix, vec3& outPosition, quat& outRotation, vec3& outScale) {
		// ��ġ ����
		outPosition.x = matrix[3][0];
		outPosition.y = matrix[3][1];
		outPosition.z = matrix[3][2];

		// ������ ����
		outScale.x = length(vec3(matrix[0][0], matrix[0][1], matrix[0][2]));
		outScale.y = length(vec3(matrix[1][0], matrix[1][1], matrix[1][2]));
		outScale.z = length(vec3(matrix[2][0], matrix[2][1], matrix[2][2]));

		// ������ ���� �� ȸ�� ����
		mat4 rotationMatrix = matrix;
		rotationMatrix[0] /= outScale.x;
		rotationMatrix[1] /= outScale.y;
		rotationMatrix[2] /= outScale.z;
		outRotation = toQuat(rotationMatrix);
	}

	inline mat4 RemoveScaleFromMatrix(const mat4& matrix) {
		vec3 scale = ExtractScale(matrix);

		mat4 result = matrix;
		result[0] /= scale.x;
		result[1] /= scale.y;
		result[2] /= scale.z;

		return result;
	}

	inline mat4 LookAt(const vec3& position, const vec3& target, const vec3& up) {
		return glm::lookAt(position, target, up);
	}

	inline mat4 ViewMatrix(const vec3& position, const vec3& rotation) {
		return LookAt(position, position + QRotate(rotation, Forward), Up);
	}

	inline mat4 Orthographic(float left, float right, float bottom, float top, float nearClip, float farClip) {
		mat4 result = ortho(left, right, bottom, top, nearClip, farClip);
		if (MathParams::InvertYAxis) {
			result[1][1] *= -1.0f; // Y축 반전
		}
		return result;
	}

	inline mat4 Perspective(float fovY, float aspectRatio, float nearClip, float farClip) {
		mat4 result = glm::perspective(fovY, aspectRatio, nearClip, farClip);
		if (MathParams::InvertYAxis) {
			result[1][1] *= -1.0f; // Y축 반전
		}
		return result;
	}

	// Screen Space to Viewport Space(NDC: -1 ~ 1)
	inline vec3 ScreenToViewport(const vec2& screenPos, const vec4& viewport) {
		return vec3((2.0f * (screenPos.x - viewport.x)) / viewport.z - 1.0f, -((2.0f * (screenPos.y - viewport.y)) / viewport.w - 1.0f), 0.0f);
	}

	// Final: Screen Space to World Space
	inline vec3 ScreenToWorld(const vec2& screenPos, const vec4& viewport, const mat4& projectionMat, const mat4& viewMat) {
		// 1. Screen -> Viewport (NDC)
		vec3 viewportPos = ScreenToViewport(screenPos, viewport);

		// 2. Viewport -> Projection
		vec4 projectionPos = glm::inverse(projectionMat) * vec4(viewportPos, 1.0f);
		projectionPos /= projectionPos.w;
		
		return glm::inverse(viewMat) * projectionPos;
	}

	inline bool GetIntersectionPos(const vec3& rayOrigin, const vec3& rayDir, const float rayLength, const vec3& planeNormal, const vec3& planePos, vec3& outPos) {
		// check 2 points are both outside or inside the plane
		vec3 startPoint = rayOrigin;
		vec3 endPoint = rayOrigin + rayDir * rayLength;

		const float dotStart = dot(planeNormal, startPoint - planePos);
		const float dotEnd = dot(planeNormal, endPoint - planePos);

		if (dotStart * dotEnd > 0.0f) {
			// both points are outside or inside the plane
			return false;
		}

		const float t = dotStart / (dotStart - dotEnd);

		outPos = glm::mix(startPoint, endPoint, t);

		return true;
	}

	inline bool IsInside(const vec3& p1, const vec3& p2, const vec3& p3, const vec3& p) {
		vec3 v0 = p2 - p1;
		vec3 v1 = p3 - p1;
		vec3 v2 = p - p1;

		// Barycentric coordinates ���
		float d00 = dot(v0, v0);
		float d01 = dot(v0, v1);
		float d11 = dot(v1, v1);
		float d20 = dot(v2, v0);
		float d21 = dot(v2, v1);

		float denom = d00 * d11 - d01 * d01;
		float v = (d11 * d20 - d01 * d21) / denom;
		float w = (d00 * d21 - d01 * d20) / denom;
		float u = 1.0f - v - w;

		// Barycentric ��ǥ ���� ��� 0~1 ������ �־�� �ﰢ�� ���ο� ����
		return (u >= 0.0f) && (v >= 0.0f) && (w >= 0.0f);
	}

	inline vec4 CalculateViewport(const vec2& contentSize, const vec2& actualSize) {
		vec2 size = contentSize;
		vec2 offset = vec2(0.0f);

		if (size.x > size.y) {
			const float ratio = actualSize.x / actualSize.y;
			size.x = size.y * ratio;
			offset.x += (contentSize.x - size.x) * 0.5f;
		}
		else {
			const float ratio = actualSize.y / actualSize.x;
			size.y = size.x * ratio;
			offset.y += (contentSize.y - size.y) * 0.5f;
		}

		return vec4(offset.x, offset.y, size.x, size.y);
	}

	template <typename T>
	inline T Remap(T min, T max, T value, T targetMin, T targetMax) {
		if (min == max) return targetMin; // ���� ó��
		using FloatT = std::conditional_t<std::is_integral_v<T>, double, T>;
		FloatT t = static_cast<FloatT>(value - min) / static_cast<FloatT>(max - min);
		t = glm::clamp(t, static_cast<FloatT>(0), static_cast<FloatT>(1)); // ���� ����
		return static_cast<T>(targetMin + t * (targetMax - targetMin));
	}

	struct Plane {
		vec4 data;

		vec3 Normal() const {
			return vec3(data.x, data.y, data.z);
		}

		float Distance() const {
			return data.w;
		}

		float Distance(const vec3& point) const {
			return dot(Normal(), point) - Distance();
		}

		static vec3 GetIntersectPoint(const Plane& p1, const Plane& p2, const Plane& p3) {
			const vec3& n1 = p1.Normal();
			const vec3& n2 = p2.Normal();
			const vec3& n3 = p3.Normal();

			float denom = dot(n1, cross(n2, n3));
			if (EpsilonEqual(denom, 0.0f)) {
				return vec3(0.0f);
			}

			vec3 c1 = cross(n2, n3) * p1.Distance();
			vec3 c2 = cross(n3, n1) * p2.Distance();
			vec3 c3 = cross(n1, n2) * p3.Distance();

			return (c1 + c2 + c3) / denom;
		}
	};

	struct Frustum {
		struct Planes {
			union {
				struct {
					Plane leftPlane;   
					Plane rightPlane;  
					Plane bottomPlane; 
					Plane topPlane;    
					Plane nearPlane;   
					Plane farPlane;    
				};
				Plane data[6];
			};
		};

		struct Corners {
			union {
				struct {
					vec3 topLeftNear;    
					vec3 topRightNear;   
					vec3 bottomRightNear; 
					vec3 bottomLeftNear; 
					vec3 topLeftFar;    
					vec3 topRightFar;    
					vec3 bottomRightFar; 
					vec3 bottomLeftFar; 
				};
				vec3 data[8];
			};
		};

		Planes planes;

		inline bool TestInside(const vec3& boundingBoxMin, const vec3& boundingBoxMax, const mat4& modelMatrix) {
			// 8 corners of the AABB
			vec3 corners[8] = {
				{boundingBoxMin.x, boundingBoxMin.y, boundingBoxMin.z},
				{boundingBoxMax.x, boundingBoxMin.y, boundingBoxMin.z},
				{boundingBoxMax.x, boundingBoxMax.y, boundingBoxMin.z},
				{boundingBoxMin.x, boundingBoxMax.y, boundingBoxMin.z},
				{boundingBoxMin.x, boundingBoxMin.y, boundingBoxMax.z},
				{boundingBoxMax.x, boundingBoxMin.y, boundingBoxMax.z},
				{boundingBoxMax.x, boundingBoxMax.y, boundingBoxMax.z},
				{boundingBoxMin.x, boundingBoxMax.y, boundingBoxMax.z}
			};

			// Transform all corners
			std::vector<vec3> transformedCorners(8);
			for (int i = 0; i < 8; ++i) {
				transformedCorners[i] = modelMatrix * vec4(corners[i], 1.0f);
			}

			// Frustum test: if all 8 points are outside any one plane, it's outside
			for (const auto& plane : planes.data) {
				int outsideCount = 0;
				for (const auto& corner : transformedCorners) {
					if (plane.Distance(corner) > 0.0f) {
						++outsideCount;
					}
				}
				if (outsideCount == 8) {
					return false; // Culled
				}
			}

			return true; // At least one point is inside all planes
		}

		inline bool TestInside(const vec3& boundingSphereCenter, float boundingSphereRadius, const mat4& modelMatrix) {
			float maxScale = glm::compMax(vec3(length2(modelMatrix[0]), length2(modelMatrix[1]), length2(modelMatrix[2])));
			maxScale = sqrt(maxScale);

			const float scaledRadius = boundingSphereRadius * maxScale;
			const vec4 transformedBoundingSphereCenter = modelMatrix * vec4(boundingSphereCenter, 1.0);

			for (const auto& plane : planes.data) {
				if (plane.Distance(transformedBoundingSphereCenter) > scaledRadius) {
					return false;
				}
			}

			return true;
		}

		inline Corners GetCorners() const {
			Corners outCorners;

			outCorners.topLeftNear = Plane::GetIntersectPoint(planes.leftPlane, planes.topPlane, planes.nearPlane); 
			outCorners.topRightNear = Plane::GetIntersectPoint(planes.rightPlane, planes.nearPlane, planes.topPlane);
			outCorners.bottomRightNear = Plane::GetIntersectPoint(planes.rightPlane, planes.bottomPlane, planes.nearPlane);
			outCorners.bottomLeftNear = Plane::GetIntersectPoint(planes.leftPlane, planes.bottomPlane, planes.nearPlane);

			outCorners.topLeftFar = Plane::GetIntersectPoint(planes.leftPlane, planes.farPlane, planes.topPlane); 
			outCorners.topRightFar = Plane::GetIntersectPoint(planes.rightPlane, planes.topPlane, planes.farPlane);
			outCorners.bottomRightFar = Plane::GetIntersectPoint(planes.rightPlane, planes.farPlane, planes.bottomPlane); 
			outCorners.bottomLeftFar = Plane::GetIntersectPoint(planes.leftPlane, planes.bottomPlane, planes.farPlane);

			return outCorners;
		}
	};

	inline void CreateFrustum(const float fovX, const float fovY, const float nearClip, const float farClip, const vec3& position, const vec3& lookDirection, Frustum& frustrum) {
		vec3 forward = lookDirection;
		vec3 right = normalize(cross(Up, forward));
		vec3 up = normalize(cross(forward, right));

		float nearHalfHeight = tan(fovY * 0.5f) * nearClip;
		float nearHalfWidth = tan(fovX * 0.5f) * nearClip;

		vec3 nearCenter = position + forward * nearClip;

		// Left plane
		auto origin = nearCenter - right * nearHalfWidth;
		auto normal = normalize(cross(origin - position, up));
		frustrum.planes.leftPlane.data = vec4(normal, dot(normal, origin));

		// Right plane
		origin = nearCenter + right * nearHalfWidth;
		normal = normalize(cross(up, origin - position));
		frustrum.planes.rightPlane.data = vec4(normal, dot(normal, origin));

		// Top plane
		origin = nearCenter + up * nearHalfHeight;
		normal = normalize(cross(origin - position, right));
		frustrum.planes.topPlane.data = vec4(normal, dot(normal, origin));

		// Bottom plane
		origin = nearCenter - up * nearHalfHeight;
		normal = normalize(cross(right, origin - position));
		frustrum.planes.bottomPlane.data = vec4(normal, dot(normal, origin));

		// Near plane
		origin = position + forward * nearClip;
		normal = -forward;
		frustrum.planes.nearPlane.data = vec4(normal, dot(normal, origin));

		// Far plane
		origin = position + forward * farClip;
		normal = forward;
		frustrum.planes.farPlane.data = vec4(normal, dot(normal, origin));
	}

	inline void CreateFrustum(const float fovX, const float fovY, const float nearClip, const float farClip, const mat4& transform, Frustum& frustrum) {
		vec3 position = ExtractPosition(transform);
		vec3 forward = normalize(vec3(transform * vec4(Forward, 0)));

		CreateFrustum(fovX, fovY, nearClip, farClip, position, forward, frustrum);
	}

	inline void CreateFrustum(const float fovX, const float fovY, const float nearClip, const float farClip, Frustum& frustrum) {
		CreateFrustum(fovX, fovY, nearClip, farClip, mat4(1.0f), frustrum);
	}
}
