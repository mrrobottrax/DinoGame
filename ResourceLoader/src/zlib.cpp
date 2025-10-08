#include "pch.h"

#include "zlib.h"

#define ZLIB_OUT_OF_MEMORY MAKE_ERROR(00, 00, 00);

#define ZLIB_HEADER_NO_SPACE MAKE_ERROR(01, 00, 00);
#define ZLIB_HEADER_FAILED_CHECK MAKE_ERROR(01, 00, 01);

#define ZLIB_HEADER_UNSUPPORTED_CM MAKE_ERROR(01, 01, 00);
#define ZLIB_HEADER_UNSUPPORTED_PRESET_DICT MAKE_ERROR(01, 01, 01);

#define ZLIB_ADLER_NOT_ENOUGH_SPACE MAKE_ERROR(02, 00, 00);

static uint32_t zlib_u32(const uint8_t *p) {
  union {
    uint32_t v = 0;
    uint8_t p1[4];
  } u;

  u.p1[0] = p[3];
  u.p1[1] = p[2];
  u.p1[2] = p[1];
  u.p1[3] = p[0];

  return u.v;
}

RESOURCE_LOADER_API int ResourceLoader_zlib_read_header(
    const void *pFile, size_t fileSize, ResourceLoader_Zlib_Header *pHeader,
    const uint32_t *pSupportedPresetDicts, size_t supportedPresetDictsLength) {
  ASSERT_RETURN(fileSize >= 2, ZLIB_HEADER_NO_SPACE);

  *pHeader = ResourceLoader_Zlib_Header{};
  pHeader->HeaderSize = 2;

  const uint8_t *file = (uint8_t *)pFile;
  pHeader->CM = file[0] & 0b00001111;
  pHeader->CINFO = file[0] & 0b11110000;

  uint32_t CMF = file[0];
  uint32_t FLG = file[1];
  ASSERT_RETURN((CMF * 256 + FLG) % 31 == 0, ZLIB_HEADER_FAILED_CHECK);

  ASSERT_RETURN(pHeader->CM == 8, ZLIB_HEADER_UNSUPPORTED_CM);

  pHeader->PresetDict = FLG & 0b100000;
  pHeader->CompressionLevel = (FLG & 0b11000000) >> 6;

  if (pHeader->PresetDict) {
    ASSERT_RETURN(fileSize >= 6, ZLIB_HEADER_NO_SPACE);
    pHeader->HeaderSize += 4;
    pHeader->DictId = zlib_u32(&file[2]);

    bool supported = false;
    if (pSupportedPresetDicts) {
      for (size_t i = 0; i < supportedPresetDictsLength; ++i) {
        if (pSupportedPresetDicts[i] == pHeader->DictId) {
          supported = true;
          break;
        }
      }
    }

    ASSERT_RETURN(supported, ZLIB_HEADER_UNSUPPORTED_PRESET_DICT);
  }

  return 0;
}

RESOURCE_LOADER_API int
ResourceLoader_zlib_read_adler(const void *pFile, size_t fileSize,
                               ResourceLoader_Zlib_Header *pHeader,
                               uint32_t *pAdler) {
  ASSERT_RETURN(fileSize >= pHeader->HeaderSize + 4,
                ZLIB_ADLER_NOT_ENOUGH_SPACE);

  *pAdler = zlib_u32((uint8_t *)pFile + fileSize - 4);

  return 0;
}
