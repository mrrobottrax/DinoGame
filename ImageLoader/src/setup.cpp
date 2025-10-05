#include "pch.h"

#include "buffer.h"
#include "setup.h"

static bool s_BufferIsSelfAllocated;

IMAGE_LOADER_API bool ImageLoader_setup(ImageLoader_SetupInfo info,
                                        ImageLoader_SetupResult *pResult) {
  if (info.BufferSize == 0) {
    return false;
  }

  if (info.pBuffer == nullptr) {
    info.pBuffer = malloc(info.BufferSize);
    if (info.pBuffer == nullptr) {
      return false;
    }

    s_BufferIsSelfAllocated = true;
  }

  if (pResult != nullptr) {
    pResult->BufferSize = info.BufferSize;
    pResult->pBuffer = info.pBuffer;
  }

  g_BufferSize = info.BufferSize;
  g_Buffer = info.pBuffer;

  return true;
}

IMAGE_LOADER_API void ImageLoader_close() {
  if (s_BufferIsSelfAllocated) {
    free(g_Buffer);
    g_Buffer = nullptr;
    g_BufferSize = 0;
  }
}