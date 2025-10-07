#pragma once

#include "arenas.h"

struct PngInfo {
  uint8_t *Data;
  size_t Size;
  uint32_t Width;
  uint32_t Height;
  uint8_t BitDepth;
  uint8_t ColorType;
  bool SRGB;
};

RESOURCE_LOADER_API int
ResourceLoader_decompress_png(const void *pFile, size_t fileSize, PngInfo *pOut,
                              ResourceLoader_arena_t arena);

RESOURCE_LOADER_API int
ResourceLoader_png_to_rgba8(const PngInfo *pSrc, PngInfo *pDst,
                            ResourceLoader_arena_t arena);