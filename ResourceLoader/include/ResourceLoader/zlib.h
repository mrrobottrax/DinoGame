#pragma once

struct ResourceLoader_Zlib_Header {
  uint32_t DictId;
  size_t HeaderSize;
  uint8_t CM;
  uint8_t CINFO;
  uint8_t CompressionLevel;
  bool PresetDict;
};

RESOURCE_LOADER_API int
ResourceLoader_zlib_read_header(const void *pFile, size_t fileSize,
                                ResourceLoader_Zlib_Header *pHeader,
                                const uint32_t *pSupportedPresetDicts = nullptr,
                                size_t supportedPresetDictsLength = 0);

RESOURCE_LOADER_API int
ResourceLoader_zlib_read_adler(const void *pFile, size_t fileSize,
                               ResourceLoader_Zlib_Header *pHeader,
                               uint32_t *pAdler);