#include "pch.h"

#include "buffer.h"
#include "png.h"

#define PNG_TOO_SMALL 1;
#define PNG_BAD_MAGIC 2;
#define PNG_INCOMPLETE 3;

static uint32_t png_u32(void *pValue) {
  uint32_t o = 0;

  char *p = (char *)pValue;
  char *p1 = (char *)&o;

  p1[0] = p[3];
  p1[1] = p[2];
  p1[2] = p[1];
  p1[3] = p[0];

  return o;
}

/// <summary>
/// Decompress png file stored in file buffer.
/// </summary>
IMAGE_LOADER_API int ResourceLoader_decompress_png(PngOutInfo *pOut) {
  uint8_t *const file = (uint8_t *)g_Buffer;
  const size_t size = g_Arena0Size;

  if (size < 8)
    return PNG_TOO_SMALL;

  uint64_t magicNum = *(uint64_t *)file;
  constexpr uint64_t correctNum = 0x0A1A0A0D474E5089;

  if (magicNum != correctNum)
    return PNG_BAD_MAGIC;

  size_t offset = 8;
  while (true) {
    if (size - offset < 12)
      return PNG_INCOMPLETE;

    size_t dataLen = png_u32(file + offset);
    char chunk[5]{};
    memcpy_s(chunk, 4, file + offset + 4, 4);

    offset += dataLen + 12;
  }

  return 0;
}
