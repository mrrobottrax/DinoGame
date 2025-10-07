#pragma once

struct ResourceLoader_SetupInfo {
  /// <summary>
  /// If nullptr, this will be allocated automatically. Buffer is used as
  /// working arena and file output.
  /// </summary>
  void *pBuffer;
  size_t BufferSize = 1 << 26; // 64MB
};

struct ResourceLoader_SetupResult {
  size_t BufferSize;
  void *pBuffer;
};

RESOURCE_LOADER_API bool
ResourceLoader_setup(ResourceLoader_SetupInfo *pInfo,
                     ResourceLoader_SetupResult *pResult);
RESOURCE_LOADER_API void ResourceLoader_close();