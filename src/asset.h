#pragma once

#include "Graphics/GraphicsBuffers.h"
#include "Graphics/GraphicsTextures.h"
#include "Math/Math.h"

#include <vector>

using namespace flaw;

struct TexturedVertex {
    vec3 position;
    vec4 color;
    vec2 texCoord;
    vec3 normal;
};

struct Material {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;

    Ref<Texture2D> diffuseTexture;
    Ref<Texture2D> specularTexture;
};

struct SubMesh {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t indexCount;
};

struct Mesh {
    Ref<VertexBuffer> vertexBuffer;
    Ref<IndexBuffer> indexBuffer;

    std::vector<SubMesh> subMeshes;
    std::vector<Ref<Material>> materials;
};

void Asset_Init();
void Asset_Cleanup();

void LoadTexture(const char* filePath, const char* key);
void LoadPrimitiveModel(const std::vector<TexturedVertex>& vertices, const std::vector<uint32_t>& indices, const char* key);
void LoadModel(const char* filePath, float scale, const char* key);

Ref<Texture2D> GetTexture2D(const char* key);
Ref<TextureCube> GetTextureCube(const char* key);
Ref<Mesh> GetMesh(const char* key);
