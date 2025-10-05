#pragma once

struct ResourceLoader_SetupInfo {
  size_t BufferSize = 1 << 20; // 1MB

  /// <summary>
  /// If nullptr, this will be allocated automatically.
  /// </summary>
  void *pBuffer;
};

struct ResourceLoader_SetupResult {
  size_t BufferSize;
  void *pBuffer;
};

IMAGE_LOADER_API bool ResourceLoader_setup(ResourceLoader_SetupInfo *pInfo,
                                           ResourceLoader_SetupResult *pResult);
IMAGE_LOADER_API void ResourceLoader_close();