#include "pch.h"

#include "arenas_private.h"
#include "setup.h"

static bool s_BufferIsSelfAllocated;

RESOURCE_LOADER_API bool
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

  g_ArenasBufferSize = pInfo->BufferSize;
  g_ArenasBuffer = pInfo->pBuffer;

  return true;
}

RESOURCE_LOADER_API void ResourceLoader_close() {
  if (s_BufferIsSelfAllocated) {
    free(g_ArenasBuffer);
    g_ArenasBuffer = nullptr;
    g_ArenasBufferSize = 0;
  }
}