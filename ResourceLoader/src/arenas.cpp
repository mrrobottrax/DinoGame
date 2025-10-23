#include "pch.h"

#include "arenas.h"
#include "arenas_private.h"

RESOURCE_LOADER_API void ResourceLoader_arena0_reset() { g_Arena0Size = 0; }
RESOURCE_LOADER_API void ResourceLoader_arena1_reset() { g_Arena1Size = 0; }

void *arena0_allocate(size_t amt) {
  if (g_ArenasBufferSize - g_Arena0Size - g_Arena1Size < amt)
    return nullptr;

  void *ret = (char *)g_ArenasBuffer + g_Arena0Size;
  g_Arena0Size += amt;

  return ret;
}

void *arena1_allocate(size_t amt) {
  if (g_ArenasBufferSize - g_Arena0Size - g_Arena1Size < amt)
    return nullptr;

  g_Arena1Size += amt;
  void *ret = (char *)g_ArenasBuffer + g_ArenasBufferSize - g_Arena1Size;

  return ret;
}
