#include "pch.h"

#include "buffer_private.h"

IMAGE_LOADER_API void ResourceLoader_get_buffer(size_t *pSize,
                                                void **ppBuffer) {
  *pSize = g_BufferSize;
  *ppBuffer = g_Buffer;
}