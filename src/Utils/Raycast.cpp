#include "pch.h"
#include "Raycast.h"

namespace flaw {
	BVHTriangle::BVHTriangle(const vec3& p0, const vec3& p1, const vec3& p2)
		: p0(p0)
		, p1(p1)
		, p2(p2)
	{
		center = (p0 + p1 + p2) / 3.0f;

		vec3 edge1 = p1 - p0;
		vec3 edge2 = p2 - p0;
		normal = glm::normalize(glm::cross(edge1, edge2));
	}

	void BVHBoundingBox::IncludePoint(const vec3& point) {
		min = glm::min(min, point);
		max = glm::max(max, point);
		center = (min + max) * 0.5f;
	}

	void BVHBoundingBox::IncludeTriangle(const BVHTriangle& triangle) {
		IncludePoint(triangle.p0);
		IncludePoint(triangle.p1);
		IncludePoint(triangle.p2);
	}

	void Split(std::vector<BVHNode>& nodes, std::vector<BVHTriangle>& triangles, int32_t parent, int32_t depth) {
		if (depth >= MaxBVHDepth || nodes[parent].triangleCount <= 1) {
			return;
		}

		// 가장 긴 축을 중심으로 자른다.
		vec3 size = nodes[parent].boundingBox.max - nodes[parent].boundingBox.min;

		// 0 : x축, 1: y축, 2: z축
		int32_t splitAxis = 0;
		if (size.y > size.x) {
			splitAxis = 1;
		}

		if (size.z > size.y) {
			splitAxis = 2;
		}

		BVHNode childANode;
		BVHNode childBNode;

		int32_t left = nodes[parent].triangleStart;
		int32_t right = nodes[parent].triangleStart + nodes[parent].triangleCount - 1;

		while (left <= right) {
			auto& tri = triangles[left];
			bool isInA = false;
			if (splitAxis == 0) isInA = tri.center.x < nodes[parent].boundingBox.center.x;
			else if (splitAxis == 1) isInA = tri.center.y < nodes[parent].boundingBox.center.y;
			else if (splitAxis == 2) isInA = tri.center.z < nodes[parent].boundingBox.center.z;

			if (isInA) {
				childANode.boundingBox.IncludeTriangle(tri);
				childANode.triangleCount++;
				left++;
			}
			else {
				childBNode.boundingBox.IncludeTriangle(tri);
				std::swap(triangles[left], triangles[right]);
				childBNode.triangleCount++;
				right--;
			}
		}

		childANode.triangleStart = nodes[parent].triangleStart;
		childBNode.triangleStart = left;

		nodes[parent].childA = nodes.size();
		nodes.push_back(childANode);

		nodes[parent].childB = nodes.size();
		nodes.push_back(childBNode);

		Split(nodes, triangles, nodes[parent].childA, depth + 1);
		Split(nodes, triangles, nodes[parent].childB, depth + 1);
	}

	void Raycast::BuildBVH(const std::function<vec3(int32_t)>& getVertex, int32_t vertexCount, std::vector<BVHNode>& nodes, std::vector<BVHTriangle>& triangles) {
		BVHNode rootNode;

		triangles.reserve(vertexCount / 3);
		for (size_t i = 0; i < vertexCount; i += 3) {
			const auto& v0 = getVertex(i);
			const auto& v1 = getVertex(i + 1);
			const auto& v2 = getVertex(i + 2);

			triangles.emplace_back(v0, v1, v2);
			rootNode.boundingBox.IncludeTriangle(triangles.back());
			rootNode.triangleCount++;
		}

		rootNode.triangleStart = 0;

		nodes.push_back(rootNode);

		Split(nodes, triangles, 0, 0);
	}

	bool Raycast::BVHBoundingBoxLineIntersect(const BVHBoundingBox& box, const Ray& ray) {
		float xMin = (box.min.x - ray.origin.x) / ray.direction.x;
		float xMax = (box.max.x - ray.origin.x) / ray.direction.x;
		if (xMin > xMax) {
			std::swap(xMin, xMax);
		}

		float yMin = (box.min.y - ray.origin.y) / ray.direction.y;
		float yMax = (box.max.y - ray.origin.y) / ray.direction.y;
		if (yMin > yMax) {
			std::swap(yMin, yMax);
		}

		float zMin = (box.min.z - ray.origin.z) / ray.direction.z;
		float zMax = (box.max.z - ray.origin.z) / ray.direction.z;
		if (zMin > zMax) {
			std::swap(zMin, zMax);
		}

		float tMin = std::max(std::max(xMin, yMin), zMin);
		float tMax = std::min(std::min(xMax, yMax), zMax);

		if (tMin > tMax) {
			return false;
		}

		return true;
	}

	bool Raycast::BVHTriangleLineIntersect(const BVHTriangle& tri, const Ray& ray, vec3& outPos, float& outT) {
		const float EPSILON = 1e-6f;

		vec3 edge1 = tri.p1 - tri.p0;
		vec3 edge2 = tri.p2 - tri.p0;
		vec3 h = cross(ray.direction, edge2);
		float a = dot(edge1, h);

		if (fabs(a) < EPSILON) {
			return false; // 평행
		}

		float f = 1.0f / a;
		vec3 s = ray.origin - tri.p0;
		float u = f * dot(s, h);
		if (u < 0.0f || u > 1.0f) {
			return false;
		}

		vec3 q = cross(s, edge1);
		float v = f * dot(ray.direction, q);
		if (v < 0.0f || u + v > 1.0f) {
			return false;
		}

		float t = f * dot(edge2, q);
		if (t < 0.0f || t > ray.length) {
			return false;
		}

		outT = t;
		outPos = ray.origin + ray.direction * t;
		return true;
	}

	bool Raycast::RayCastBVHImpl(const std::vector<BVHNode>& nodes, const std::vector<BVHTriangle>& triangles, const int32_t current, const Ray& ray, RayHit& hit) {
		const auto& node = nodes[current];

		if (!BVHBoundingBoxLineIntersect(node.boundingBox, ray)) {
			return false;
		}

		bool result = false;

		if (node.IsLeaf()) {
			for (int32_t i = 0; i < node.triangleCount; ++i) {
				auto& tri = triangles[node.triangleStart + i];

				vec3 hitPoint;
				float t;
				if (BVHTriangleLineIntersect(tri, ray, hitPoint, t)) {
					if (t < hit.distance) {
						hit.position = hitPoint;
						hit.normal = tri.normal;
						hit.distance = t;
						result = true;
					}
				}
			}
		}
		else {
			RayHit leftHit = hit;
			RayHit rightHit = hit;

			bool leftResult = node.childA != -1 && RayCastBVHImpl(nodes, triangles, node.childA, ray, leftHit);
			bool rightResult = node.childB != -1 && RayCastBVHImpl(nodes, triangles, node.childB, ray, rightHit);

			if (leftResult && rightResult) {
				hit = (leftHit.distance < rightHit.distance) ? leftHit : rightHit;
				result = true;
			}
			else if (leftResult) {
				hit = leftHit;
				result = true;
			}
			else if (rightResult) {
				hit = rightHit;
				result = true;
			}
		}

		return result;
	}

	bool Raycast::RaycastBVH(const std::vector<BVHNode>& nodes, const std::vector<BVHTriangle>& triangles, const Ray& ray, RayHit& hit) {
		return RayCastBVHImpl(nodes, triangles, 0, ray, hit);
	}

	void Raycast::GetCandidateBVHTrianglesImpl(const std::vector<BVHNode>& nodes, const std::vector<BVHTriangle>& triangles, const Ray& ray, int32_t current, const std::function<void(int32_t, int32_t)>& callback) {
		const auto& node = nodes[current];

		if (!BVHBoundingBoxLineIntersect(node.boundingBox, ray)) {
			return;
		}

		if (node.IsLeaf()) {
			callback(node.triangleStart, node.triangleCount);
		}
		else {
			if (node.childA != -1) {
				GetCandidateBVHTrianglesImpl(nodes, triangles, ray, node.childA, callback);
			}
			if (node.childB != -1) {
				GetCandidateBVHTrianglesImpl(nodes, triangles, ray, node.childB, callback);
			}
		}
	}

	void Raycast::GetCandidateBVHTriangles(const std::vector<BVHNode>& nodes, const std::vector<BVHTriangle>& triangles, const Ray& ray, const std::function<void(int32_t, int32_t)>& callback) {
		GetCandidateBVHTrianglesImpl(nodes, triangles, ray, 0, callback);
	}
}