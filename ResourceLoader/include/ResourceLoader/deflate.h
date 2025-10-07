#pragma once

enum EResourceLoader_Deflate_Stage {
  RESOURCE_LOADER_DEFLATE_STAGE_INITIAL,
  RESOURCE_LOADER_DEFLATE_STAGE_READ_HEADER,
  RESOURCE_LOADER_DEFLATE_STAGE_READ_HUFFMAN_TREE,
};

struct ResourceLoader_Deflate_State {
  uint8_t *pOutStream;
  size_t OutStreamSize;

  size_t OutStreamByteOffset;

  EResourceLoader_Deflate_Stage Stage;

  uint8_t CompressionType;

  uint8_t OutStreamBitOffset;

  bool IsFinalChunk;
};

// Stream size is in bytes. It's likely that the end bit of the DEFLATE stream
// doesn't land on a byte boundary. That's fine as long as the byte stream
// contains the entire DEFLATE stream.
RESOURCE_LOADER_API int
ResourceLoader_deflate_read_partial(const uint8_t *pStream, size_t streamSize,
                                    ResourceLoader_Deflate_State *pState);