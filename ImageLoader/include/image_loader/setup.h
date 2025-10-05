#pragma once

struct ImageLoader_SetupInfo {
  size_t BufferSize = 1 << 20; // 1MB

  /// <summary>
  /// If nullptr, this will be allocated automatically.
  /// </summary>
  void *pBuffer;
};

struct ImageLoader_SetupResult {
  size_t BufferSize;
  void *pBuffer;
};

IMAGE_LOADER_API bool image_loader_setup(ImageLoader_SetupInfo info,
                                         ImageLoader_SetupResult *pResult);
IMAGE_LOADER_API void image_loader_close();