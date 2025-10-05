#include "pch.h"

#include "buffer.h"
#include "setup.h"

static bool s_BufferIsSelfAllocated;

IMAGE_LOADER_API bool
ResourceLoader_setup(ResourceLoader_SetupInfo *pInfo,
                     ResourceLoader_SetupResult *pResult) {
  if (!pInfo) {
    return false;
  }

  if (pInfo->BufferSize == 0) {
    return false;
  }

  if (pInfo->pBuffer == nullptr) {
    pInfo->pBuffer = malloc(pInfo->BufferSize);
    if (pInfo->pBuffer == nullptr) {
      return false;
    }

    s_BufferIsSelfAllocated = true;
  }

  if (pResult != nullptr) {
    pResult->BufferSize = pInfo->BufferSize;
    pResult->pBuffer = pInfo->pBuffer;
  }

  g_BufferSize = pInfo->BufferSize;
  g_Buffer = pInfo->pBuffer;

  return true;
}

IMAGE_LOADER_API void ResourceLoader_close() {
  if (s_BufferIsSelfAllocated) {
    free(g_Buffer);
    g_Buffer = nullptr;
    g_BufferSize = 0;
  }
}