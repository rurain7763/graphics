#pragma once

#include "EngineCore.h"
#include "Platform/PlatformContext.h"
#include "Event/EventDispatcher.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsHelper.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "EngineCamera.h"
#include "object.h"
#include "RenderQueue.h"

using namespace flaw;

#define USE_VULKAN 1
#define USE_DX11 0

#define MAX_DIRECTIONAL_LIGHTS 1
#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS 8

constexpr uint32_t MaxInstancingCount = 10000;

struct Material;

extern Ref<PlatformContext> g_context;
extern EventDispatcher g_eventDispatcher;
extern Ref<GraphicsContext> g_graphicsContext;

extern Ref<EngineCamera> g_camera;
extern std::vector<Object> g_objects;
extern std::vector<uint32_t> g_outlineObjects;
extern std::vector<uint32_t> g_viewNormalObjects;
extern std::vector<uint32_t> g_spriteObjects;

extern RenderQueue g_renderQueue;

extern std::vector<Ref<Framebuffer>> g_sceneFramebuffers;
extern Ref<RenderPassLayout> g_sceneRenderPassLayout;
extern Ref<RenderPass> g_sceneClearRenderPass;
extern Ref<RenderPass> g_sceneLoadRenderPass;
extern Ref<VertexInputLayout> g_texturedVertexInputLayout;
extern Ref<VertexInputLayout> g_instanceVertexInputLayout;
extern Ref<ConstantBuffer> g_cameraCB;
extern Ref<ConstantBuffer> g_globalCB;
extern Ref<ConstantBuffer> g_lightCB;
extern Ref<StructuredBuffer> g_directionalLightSB;
extern Ref<StructuredBuffer> g_pointLightSB;
extern Ref<StructuredBuffer> g_spotLightSB;

extern Ref<GraphicsResourcesPool<VertexBuffer>> g_instanceVBPool;
extern Ref<GraphicsResourcesPool<ConstantBuffer>> g_objMaterialCBPool;
extern Ref<GraphicsResourcesPool<ConstantBuffer>> g_objConstantsCBPool;

void World_Init();
void World_Cleanup();
void World_Update();
void World_Render();
void World_FinalizeRender();

void Shadow_Init();
void Shadow_Cleanup();
void Shadow_Update();
void Shadow_Render();

void Skybox_Init();
void Skybox_Cleanup();
void Skybox_Render();

void Geometry_Init();
void Geometry_Cleanup();
void Geometry_Render();

Object& AddObject();
Object& GetObjectWithName(const char* name);

MaterialConstants GetMaterialConstants(Ref<Material> material);

std::vector<uint8_t> GenerateTextureCubeData(Image& left, Image& right, Image& top, Image& bottom, Image& front, Image& back);