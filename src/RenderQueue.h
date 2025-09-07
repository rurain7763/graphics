#pragma once

#include "EngineCore.h"
#include "Graphics/GraphicsBuffers.h"

#include <vector>

struct MeshKey {
	Ref<Mesh> mesh;
	int32_t segmentIndex = 0;

	bool operator==(const MeshKey& other) const {
		return mesh == other.mesh && segmentIndex == other.segmentIndex;
	};
};

struct SkeletalMeshKey {
	Ref<Mesh> mesh;
	int32_t segmentIndex = 0;
	Ref<StructuredBuffer> skeletonBoneMatrices;

	bool operator==(const SkeletalMeshKey& other) const {
		return mesh == other.mesh && segmentIndex == other.segmentIndex && skeletonBoneMatrices == other.skeletonBoneMatrices;
	}
};

namespace std {
	template<>
	struct hash<MeshKey> {
		size_t operator()(const MeshKey& key) const {
			return hash<flaw::Ref<Mesh>>()(key.mesh) ^ hash<int32_t>()(key.segmentIndex);
		}
	};

	template <>
	struct hash<SkeletalMeshKey> {
		size_t operator()(const SkeletalMeshKey& key) const {
			return hash<flaw::Ref<Mesh>>()(key.mesh) ^ hash<int32_t>()(key.segmentIndex) ^ hash<Ref<flaw::StructuredBuffer>>()(key.skeletonBoneMatrices);
		}
	};
}

struct InstanceData {
	mat4 model_matrix;
	mat4 inv_model_matrix;
};

struct InstancingObject {
	Ref<Mesh> mesh;
	int32_t segmentIndex;

	std::vector<InstanceData> instanceDatas;
	uint32_t instanceCount;

	inline bool HasSegment() const { return segmentIndex != -1; }
};

struct SkeletalInstancingObject {
	Ref<Mesh> mesh;
	int32_t segmentIndex;

	std::vector<InstanceData> instanceDatas;
	Ref<StructuredBuffer> skeletonBoneMatrices;
	uint32_t instanceCount;

	inline bool HasSegment() const { return segmentIndex != -1; }
};

class RenderQueue {
public:
	struct Entry {
		Ref<Material> material;

		std::unordered_map<MeshKey, int32_t> instancingIndexMap;
		std::vector<InstancingObject> instancingObjects;

		std::unordered_map<SkeletalMeshKey, int32_t> skeletalInstancingIndexMap;
		std::vector<SkeletalInstancingObject> skeletalInstancingObjects;
	};

	RenderQueue() = default;

	void Open();
	void Close();

	void Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material);
	void Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material, const Ref<StructuredBuffer>& boneMatrices);
	void Push(const Ref<Mesh>& mesh, const mat4& worldMat);
	void Push(const Ref<Mesh>& mesh, const mat4& worldMat, const Ref<Material>& material);
	void Push(const Ref<Mesh>& mesh, const mat4& worldMat, const Ref<Material>& material, const Ref<StructuredBuffer>& boneMatrices);
	
	void Next();

	void Reset();

	bool Empty();
	void Clear();

	Entry& Front();

	inline const std::vector<InstanceData>& AllInstanceDatas() const { return _allInstanceDatas; }

private:
	int32_t GetRenderEntryIndex(const Ref<Material>& material);

private:
	std::vector<InstanceData> _allInstanceDatas;

	std::unordered_map<Ref<Material>, int32_t> _entryIndexMap;
	std::vector<Entry> _entries;

	uint32_t _currentEntryIndex = 0;
};
