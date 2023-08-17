#pragma once

#include "VirtualMethod.h"
#include "ITexture.h"
class KeyValues;
class Material;
class RenderContext;
#define TEXTURE_GROUP_LIGHTMAP						            "Lightmaps"
#define TEXTURE_GROUP_WORLD							            "World textures"
#define TEXTURE_GROUP_MODEL							            "Model textures" 
#define TEXTURE_GROUP_VGUI							            "VGUI textures"
#define TEXTURE_GROUP_PARTICLE						            "Particle textures"
#define TEXTURE_GROUP_DECAL							            "Decal textures"
#define TEXTURE_GROUP_SKYBOX						            "SkyBox textures"
#define TEXTURE_GROUP_CLIENT_EFFECTS				            "ClientEffect textures"
#define TEXTURE_GROUP_OTHER							            "Other textures" 
#define TEXTURE_GROUP_PRECACHED						            "Precached"
#define TEXTURE_GROUP_CUBE_MAP						            "CubeMap textures"
#define TEXTURE_GROUP_RENDER_TARGET					            "RenderTargets"
#define TEXTURE_GROUP_UNACCOUNTED					            "Unaccounted textures"
#define TEXTURE_GROUP_STATIC_PROP                               "StaticProp textures"
#define TEXTURE_GROUP_STATIC_INDEX_BUFFER			            "Static Indices"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_DISP		            "Displacement Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_COLOR	            "Lighting Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_WORLD	            "World Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_MODELS	            "Model Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_OTHER	            "Other Verts"
#define TEXTURE_GROUP_DYNAMIC_INDEX_BUFFER			            "Dynamic Indices"
#define TEXTURE_GROUP_DYNAMIC_VERTEX_BUFFER			            "Dynamic Verts"
#define TEXTURE_GROUP_DEPTH_BUFFER					            "DepthBuffer"
#define TEXTURE_GROUP_VIEW_MODEL					            "ViewModel"
#define TEXTURE_GROUP_PIXEL_SHADERS					            "Pixel Shaders"
#define TEXTURE_GROUP_VERTEX_SHADERS				            "Vertex Shaders"
#define TEXTURE_GROUP_RENDER_TARGET_SURFACE			            "RenderTarget Surfaces"
#define TEXTURE_GROUP_MORPH_TARGETS					            "Morph Targets"

enum CompiledVtfFlags
{
    TEXTUREFLAGS_POINTSAMPLE = 0x00000001,
    TEXTUREFLAGS_TRILINEAR = 0x00000002,
    TEXTUREFLAGS_CLAMPS = 0x00000004,
    TEXTUREFLAGS_CLAMPT = 0x00000008,
    TEXTUREFLAGS_ANISOTROPIC = 0x00000010,
    TEXTUREFLAGS_HINT_DXT5 = 0x00000020,
    TEXTUREFLAGS_PWL_CORRECTED = 0x00000040,
    TEXTUREFLAGS_NORMAL = 0x00000080,
    TEXTUREFLAGS_NOMIP = 0x00000100,
    TEXTUREFLAGS_NOLOD = 0x00000200,
    TEXTUREFLAGS_ALL_MIPS = 0x00000400,
    TEXTUREFLAGS_PROCEDURAL = 0x00000800,
    TEXTUREFLAGS_ONEBITALPHA = 0x00001000,
    TEXTUREFLAGS_EIGHTBITALPHA = 0x00002000,
    TEXTUREFLAGS_ENVMAP = 0x00004000,
    TEXTUREFLAGS_RENDERTARGET = 0x00008000,
    TEXTUREFLAGS_DEPTHRENDERTARGET = 0x00010000,
    TEXTUREFLAGS_NODEBUGOVERRIDE = 0x00020000,
    TEXTUREFLAGS_SINGLECOPY = 0x00040000,
    TEXTUREFLAGS_PRE_SRGB = 0x00080000,
    TEXTUREFLAGS_UNUSED_00100000 = 0x00100000,
    TEXTUREFLAGS_UNUSED_00200000 = 0x00200000,
    TEXTUREFLAGS_UNUSED_00400000 = 0x00400000,
    TEXTUREFLAGS_NODEPTHBUFFER = 0x00800000,
    TEXTUREFLAGS_UNUSED_01000000 = 0x01000000,
    TEXTUREFLAGS_CLAMPU = 0x02000000,
    TEXTUREFLAGS_VERTEXTEXTURE = 0x04000000,
    TEXTUREFLAGS_SSBUMP = 0x08000000,
    TEXTUREFLAGS_UNUSED_10000000 = 0x10000000,
    TEXTUREFLAGS_BORDER = 0x20000000,
    TEXTUREFLAGS_UNUSED_40000000 = 0x40000000,
    TEXTUREFLAGS_UNUSED_80000000 = 0x80000000
};

enum MaterialRenderTargetDepth_t
{
    MATERIAL_RT_DEPTH_SHARED = 0x0,
    MATERIAL_RT_DEPTH_SEPARATE = 0x1,
    MATERIAL_RT_DEPTH_NONE = 0x2,
    MATERIAL_RT_DEPTH_ONLY = 0x3,
};

enum RenderTargetSizeMode_t
{
    RT_SIZE_NO_CHANGE = 0,
    RT_SIZE_DEFAULT = 1,
    RT_SIZE_PICMIP = 2,
    RT_SIZE_HDR = 3,
    RT_SIZE_FULL_FRAME_BUFFER = 4,
    RT_SIZE_OFFSCREEN = 5,
    RT_SIZE_FULL_FRAME_BUFFER_ROUNDED_UP = 6
};

#define CREATERENDERTARGETFLAGS_HDR				0x00000001
#define CREATERENDERTARGETFLAGS_AUTOMIPMAP		0x00000002
#define CREATERENDERTARGETFLAGS_UNFILTERABLE_OK 0x00000004
class MaterialSystem {
public:
    VIRTUAL_METHOD(int, GetBackBufferFormat, 36, (), (this))
    VIRTUAL_METHOD(Material*, createMaterial, 83, (const char* materialName, KeyValues* keyValues), (this, materialName, keyValues))
    VIRTUAL_METHOD(Material*, findMaterial, 84, (const char* materialName, const char* textureGroupName = nullptr, bool complain = true, const char* complainPrefix = nullptr), (this, materialName, textureGroupName, complain, complainPrefix))
    VIRTUAL_METHOD(short, firstMaterial, 86, (), (this))
    VIRTUAL_METHOD(short, nextMaterial, 87, (short handle), (this, handle))
    VIRTUAL_METHOD(short, invalidMaterial, 88, (), (this))
    VIRTUAL_METHOD(Material*, getMaterial, 89, (short handle), (this, handle))
    VIRTUAL_METHOD(void, BeginRenderTargetAllocation, 94, (), (this))
    VIRTUAL_METHOD(ITexture*, CreateNamedRenderTargetTextureEx, 97, (const char* szName, int iWidth, int iHeight, RenderTargetSizeMode_t sizeMode, int format, MaterialRenderTargetDepth_t depth = MATERIAL_RT_DEPTH_SHARED, unsigned int fTextureFlags = 0U, unsigned int fRenderTargetFlags = CREATERENDERTARGETFLAGS_HDR), (this, szName, iWidth, iHeight, sizeMode, format, depth, fTextureFlags, fRenderTargetFlags))
    VIRTUAL_METHOD(RenderContext*, getRenderContext, 115, (), (this))
    VIRTUAL_METHOD(void, FinishRenderTargetAllocation, 136, (), (this))
};
