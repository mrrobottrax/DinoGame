#pragma once

#include "arenas.h"

inline void *g_ArenasBuffer;
inline size_t g_ArenasBufferSize;

/// <summary>
/// Counts up from bottom of arena.
/// </summary>
inline size_t g_Arena0Size;

/// <summary>
/// Counts down from top of arena.
/// </summary>
inline size_t g_Arena1Size;

inline void arena0_reset() { g_Arena0Size = 0; }
inline void arena1_reset() { g_Arena1Size = 0; }

void *arena0_allocate(size_t amt);
void *arena1_allocate(size_t amt);

inline void arena_reset(ResourceLoader_arena_t arena) {
  if (arena == 0)
    return arena0_reset();
  if (arena == 1)
    return arena1_reset();
  CRASH("Invalid arena id");
}

inline void *arena_allocate(ResourceLoader_arena_t arena, size_t amt) {
  if (arena == 0)
    return arena0_allocate(amt);
  if (arena == 1)
    return arena1_allocate(amt);
  CRASH("Invalid arena id");
}