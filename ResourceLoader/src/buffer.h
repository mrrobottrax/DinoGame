#pragma once

inline void *g_Buffer;
inline size_t g_BufferSize;

/// <summary>
/// Counts up from bottom of arena.
/// </summary>
inline size_t g_Arena0Size;

/// <summary>
/// Counts down from top of arena.
/// </summary>
inline size_t g_Arena1Size;

void arena0_reset();
void arena1_reset();

void *arena0_allocate(size_t amt);
void *arena1_allocate(size_t amt);