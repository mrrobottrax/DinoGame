#include "pch.h"

#include "buffer.h"

IMAGE_LOADER_API void ImageLoader_get_buffer(size_t *pSize, void **ppBuffer) {
  *pSize = g_BufferSize;
  *ppBuffer = g_Buffer;
}