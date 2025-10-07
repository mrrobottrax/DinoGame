#pragma once

struct ResourceLoader_ZlibHeader {
  uint8_t CM;
  uint8_t CINFO;
  uint8_t CompressionLevel;
  uint32_t DictId;
  bool PresetDict;
};

RESOURCE_LOADER_API int
ResourceLoader_zlib_read_header(const void *pFile, size_t fileSize,
                                ResourceLoader_ZlibHeader *pHeader,
                                const uint32_t *pSupportedPresetDicts = nullptr,
                                size_t supportedPresetDictsLength = 0);

RESOURCE_LOADER_API int
ResourceLoader_zlib_read_adler(const void *pFile, size_t fileSize,
                               ResourceLoader_ZlibHeader *pHeader,
                               uint32_t *pAdler);