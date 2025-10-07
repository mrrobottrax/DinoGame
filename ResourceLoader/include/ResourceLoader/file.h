#pragma once

#include "arenas.h"

/// <summary>
/// Load file into memory.
/// </summary>
RESOURCE_LOADER_API int ResourceLoader_load_file(const char *path,
                                                 void **ppFile,
                                                 size_t *pFileSize,
                                                 ResourceLoader_arena_t arena);