#pragma once

struct PngOutInfo {
  uint32_t Width;
  uint32_t Height;
  unsigned char *Data;
};

IMAGE_LOADER_API int ResourceLoader_decompress_png(PngOutInfo *pOut);