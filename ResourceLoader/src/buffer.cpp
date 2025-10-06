#include "pch.h"

#include "buffer.h"

/// <summary>
/// Counts up from bottom of arena.
/// </summary>
static size_t s_Arena0Size;

/// <summary>
/// Counts down from top of arena.
/// </summary>
static size_t s_Arena1Size;

void arena0_reset() { s_Arena0Size = 0; }
void arena1_reset() { s_Arena1Size = 0; }

void *arena0_allocate(size_t amt) {
  if (g_BufferSize - s_Arena0Size - s_Arena1Size < amt)
    return nullptr;

  void *ret = (char *)g_Buffer + s_Arena0Size;
  s_Arena0Size += amt;

  return ret;
}

void *arena1_allocate(size_t amt) {
  if (g_BufferSize - s_Arena0Size - s_Arena1Size < amt)
    return nullptr;

  s_Arena1Size += amt;
  void *ret = (char *)g_Buffer + g_BufferSize - s_Arena1Size;

  return ret;
}
