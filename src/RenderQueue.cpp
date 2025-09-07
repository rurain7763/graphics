#include "pch.h"
#include "RenderQueue.h"

void RenderQueue::Open() {
	_entryIndexMap.clear();
	_entries.clear();
	_currentEntryIndex = 0;
}

void RenderQueue::Close() {
	_currentEntryIndex = 0;

	size_t totalInstanceCount = 0;
	for (const auto& entry : _entries) {
		for (const auto& instance : entry.instancingObjects) {
			totalInstanceCount += instance.instanceDatas.size();
		}
	}

	_allInstanceDatas.resize(totalInstanceCount);

	size_t offset = 0;
	for (const auto& entry : _entries) {
		for (const auto& instance : entry.instancingObjects) {
			size_t instanceSize = instance.instanceDatas.size();
			std::memcpy(_allInstanceDatas.data() + offset, instance.instanceDatas.data(), sizeof(InstanceData) * instanceSize);
			offset += instanceSize;
		}
	}
}

int32_t RenderQueue::GetRenderEntryIndex(const Ref<Material>& material) {
	int32_t entryIndex = -1;

	auto indexIt = _entryIndexMap.find(material);
	if (indexIt == _entryIndexMap.end()) {
		entryIndex = _entries.size();
		_entries.resize(entryIndex + 1);

		auto& entry = _entries.back();
		entry.material = material;

		_entryIndexMap[material] = entryIndex;
	}
	else {
		entryIndex = indexIt->second;
	}

	return entryIndex;
}

void RenderQueue::Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material) {
	int32_t entryIndex = GetRenderEntryIndex(material);

	auto& entry = _entries[entryIndex];

	MeshKey meshKey{ mesh, segmentIndex };
	int32_t instanceIndex = -1;

	auto instancingIndexIt = entry.instancingIndexMap.find(meshKey);
	if (instancingIndexIt == entry.instancingIndexMap.end()) {
		InstancingObject instance;
		instance.mesh = mesh;
		instance.segmentIndex = segmentIndex;
		instance.instanceCount = 0;

		instanceIndex = entry.instancingObjects.size();
		entry.instancingIndexMap[meshKey] = instanceIndex;
		entry.instancingObjects.emplace_back(instance);
	}
	else {
		instanceIndex = instancingIndexIt->second;
	}

	auto& instance = entry.instancingObjects[instanceIndex];
	instance.instanceDatas.emplace_back(InstanceData{ worldMat, inverse(worldMat) });
	instance.instanceCount++;
}

void RenderQueue::Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material, const Ref<StructuredBuffer>& boneMatrices) {
	int32_t entryIndex = GetRenderEntryIndex(material);

	auto& entry = _entries[entryIndex];

	SkeletalMeshKey meshKey{ mesh, segmentIndex, boneMatrices };
	int32_t instanceIndex = -1;

	auto skeletalInstancingIndexIt = entry.skeletalInstancingIndexMap.find(meshKey);
	if (skeletalInstancingIndexIt == entry.skeletalInstancingIndexMap.end()) {
		SkeletalInstancingObject instance;
		instance.mesh = mesh;
		instance.segmentIndex = segmentIndex;
		instance.skeletonBoneMatrices = boneMatrices;

		instanceIndex = entry.skeletalInstancingObjects.size();
		entry.skeletalInstancingIndexMap[meshKey] = instanceIndex;
		entry.skeletalInstancingObjects.emplace_back(instance);
	}
	else {
		instanceIndex = skeletalInstancingIndexIt->second;
	}

	auto& instance = entry.skeletalInstancingObjects[instanceIndex];
	instance.instanceDatas.emplace_back(InstanceData{ worldMat, inverse(worldMat) });
	instance.instanceCount++;
}

void RenderQueue::Push(const Ref<Mesh>& mesh, const mat4& worldMat) {
	Push(mesh, -1, worldMat, nullptr);
}

void RenderQueue::Push(const Ref<Mesh>& mesh, const mat4& worldMat, const Ref<Material>& material) {
	int32_t segmentIdx = 0;
	for (auto& segment : mesh->segments) {
		Push(mesh, segmentIdx, worldMat, material);
		segmentIdx++;
	}
}

void RenderQueue::Push(const Ref<Mesh>& mesh, const mat4& worldMat, const Ref<Material>& material, const Ref<StructuredBuffer>& boneMatrices) {
	int32_t segmentIdx = 0;
	for (auto& segment : mesh->segments) {
		Push(mesh, segmentIdx, worldMat, material, boneMatrices);
		segmentIdx++;
	}
}

void RenderQueue::Next() {
	_currentEntryIndex++;
}

void RenderQueue::Reset() {
	_currentEntryIndex = 0;
}

bool RenderQueue::Empty() {
	return _currentEntryIndex >= _entries.size();
}

void RenderQueue::Clear() {
	_entryIndexMap.clear();
	_entries.clear();
}

RenderQueue::Entry& RenderQueue::Front() {
	return _entries[_currentEntryIndex];
}

