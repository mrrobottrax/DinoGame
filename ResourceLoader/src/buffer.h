#pragma once

inline void *g_Buffer;
inline size_t g_BufferSize;

void arena0_reset();
void arena1_reset();

void *arena0_allocate(size_t amt);
void *arena1_allocate(size_t amt);