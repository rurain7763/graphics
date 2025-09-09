#pragma once

#include "EngineCore.h"

#include <vector>
#include <array>

void Asset_Init();
void Asset_Cleanup();

void LoadTexture(const char* filePath, PixelFormat pixelFormat, const char* key);
void LoadTextureCube(const std::array<const char*, 6>& faceFilePaths, const char* key);
void LoadPrimitiveModel(const std::vector<TexturedVertex>& vertices, const std::vector<uint32_t>& indices, const char* key);
void LoadModel(const char* filePath, float scale, const char* key);
void LoadMaterial(const char* key);

Ref<Texture2D> GetTexture2D(const char* key);
Ref<TextureCube> GetTextureCube(const char* key);
Ref<Mesh> GetMesh(const char* key);
Ref<Material> GetMaterial(const char* key);
