#include "pch.h"

#include "buffer.h"

void arena0_reset() { g_Arena0Size = 0; }
void arena1_reset() { g_Arena1Size = 0; }

void *arena0_allocate(size_t amt) {
  if (g_BufferSize - g_Arena0Size - g_Arena1Size < amt)
    return nullptr;

  void *ret = (char *)g_Buffer + g_Arena0Size;
  g_Arena0Size += amt;

  return ret;
}

void *arena1_allocate(size_t amt) {
  if (g_BufferSize - g_Arena0Size - g_Arena1Size < amt)
    return nullptr;

  g_Arena1Size += amt;
  void *ret = (char *)g_Buffer + g_BufferSize - g_Arena1Size;

  return ret;
}
