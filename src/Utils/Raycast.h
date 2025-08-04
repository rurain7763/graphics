#pragma once

#include "Math/Math.h"

#include <functional>

namespace flaw {
	constexpr int32_t MaxBVHDepth = 3;

	struct Ray {
		vec3 origin;
		vec3 direction;
		float length = 0.0f;
	};

	struct RayHit {
		vec3 position;
		vec3 normal;
		float distance = std::numeric_limits<float>::max();
	};

	struct BVHTriangle {
		vec3 p0, p1, p2;
		vec3 center;
		vec3 normal;

		BVHTriangle(const vec3& p0, const vec3& p1, const vec3& p2);
	};

	struct BVHBoundingBox {
		vec3 min = vec3(std::numeric_limits<float>::max());
		vec3 max = vec3(std::numeric_limits<float>::lowest());
		vec3 center = vec3(0.0);

		void IncludePoint(const vec3& point);
		void IncludeTriangle(const BVHTriangle& triangle);
	};

	struct BVHNode {
		BVHBoundingBox boundingBox;
		int32_t triangleStart = -1;
		int32_t triangleCount = 0;
		int32_t childA = -1;
		int32_t childB = -1;

		bool IsLeaf() const { return childA == -1 && childB == -1; }
	};

	class Raycast {
	public:
		static void BuildBVH(const std::function<vec3(int32_t)>& getVertex, int32_t vertexCount, std::vector<BVHNode>& nodes, std::vector<BVHTriangle>& triangles);
	
		static bool BVHBoundingBoxLineIntersect(const BVHBoundingBox& box, const Ray& ray);
		static bool BVHTriangleLineIntersect(const BVHTriangle& tri, const Ray& ray, vec3& outPos, float& outT);

		static bool RaycastBVH(const std::vector<BVHNode>& nodes, const std::vector<BVHTriangle>& triangles, const Ray& ray, RayHit& hit);

		static void GetCandidateBVHTriangles(const std::vector<BVHNode>& nodes, const std::vector<BVHTriangle>& triangles, const Ray& ray, const std::function<void(int32_t, int32_t)>& callback);
	
	private:
		static bool RayCastBVHImpl(const std::vector<BVHNode>& nodes, const std::vector<BVHTriangle>& triangles, const int32_t current, const Ray& ray, RayHit& hit);
	
		static void GetCandidateBVHTrianglesImpl(const std::vector<BVHNode>& nodes, const std::vector<BVHTriangle>& triangles, const Ray& ray, int32_t current, const std::function<void(int32_t, int32_t)>& callback);
	};
}