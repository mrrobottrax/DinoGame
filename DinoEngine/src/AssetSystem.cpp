#include "pch.h"

#include "AssetSystem.h"
#include "GameDllSystem.h"
#include "RenderingSystem.h"

DINO_API IAssetSystem *g_IAssetSystem = &g_AssetSystem;

static Asset_Texture s_DefaultTexture;

void AssetSystem::start() {
  s_DefaultTexture = load_texture("missing.png");
  m_IsInitialized = true;
}

void AssetSystem::stop() { m_IsInitialized = false; }

Asset_Texture AssetSystem::load_texture(const char *path) {
  ASSERT(g_RenderingSystem.is_initialized());
  ASSERT(g_GameDllSystem.is_initialized());

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  ResourceLoader_arena_t nextArena = ResourceLoader_arena0;
  ResourceLoader_arena_t prevArena = ResourceLoader_arena1;
  ResourceLoader_arena_t tempArena{};

  void *pFile;
  size_t fileSize;
  if (ResourceLoader_load_file(path, &pFile, &fileSize, nextArena) != 0) {
    console_error("Failed to load texture %s", path);
    return s_DefaultTexture;
  }

  // swap
  tempArena = nextArena;
  nextArena = prevArena;
  prevArena = tempArena;

  PngInfo png{};
  ASSERT_CODE_ALWAYS(
      ResourceLoader_decompress_png(pFile, fileSize, &png, nextArena));
  ResourceLoader_arena_reset(prevArena);

  // swap
  tempArena = nextArena;
  nextArena = prevArena;
  prevArena = tempArena;

  if (png.InterlaceMethod != 0) {
    ASSERT_CODE_ALWAYS(ResourceLoader_deinterlace_png(&png, nextArena));
    ResourceLoader_arena_reset(prevArena);

    // swap
    tempArena = nextArena;
    nextArena = prevArena;
    prevArena = tempArena;
  }

  ASSERT_CODE_ALWAYS(ResourceLoader_png_to_rgba8(&png, nextArena));

  D3D12_CPU_DESCRIPTOR_HANDLE cpu;
  D3D12_GPU_DESCRIPTOR_HANDLE gpu;
  ID3D12Resource *resource = g_RenderingSystem.upload_static_image_rgba(
      png.Width, png.Height, png.Data, &cpu, &gpu);

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  return {
      .Handle = gpu,
      .CpuHandle = cpu,
      .Resource = resource,
      .Width = png.Width,
      .Height = png.Height,
  };
}