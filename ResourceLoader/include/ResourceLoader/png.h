#pragma once

#include "arenas.h"

struct PngInfo {
  uint8_t *Data;
  uint8_t *Palette;
  uint8_t *AlphaPalette;
  uint32_t Width;
  uint32_t Height;
  uint32_t PaletteCount;
  uint8_t BitDepth;
  uint8_t ColorType;
  uint8_t InterlaceMethod;
};

RESOURCE_LOADER_API code_t
ResourceLoader_decompress_png(const void *pFile, size_t fileSize, PngInfo *pOut,
                              ResourceLoader_arena_t arena);

RESOURCE_LOADER_API code_t
ResourceLoader_deinterlace_png(PngInfo *pPng, ResourceLoader_arena_t arena);

RESOURCE_LOADER_API code_t
ResourceLoader_png_to_rgba8(PngInfo *pPng, ResourceLoader_arena_t arena);