#pragma once

enum EResourceLoader_Deflate_Stage {
  RESOURCE_LOADER_DEFLATE_STAGE_INITIAL,
  RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL,
  RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE,
  RESOURCE_LOADER_DEFLATE_STAGE_T0_COPY_DATA,
  RESOURCE_LOADER_DEFLATE_STAGE_T1_SETUP_STATIC_TREE,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READ_HUFFMAN_TREE,
  RESOURCE_LOADER_DEFLATE_STAGE_DECODE,
};

struct ResourceLoader_Deflate_State_T0 {
  uint16_t LEN;
  uint16_t NLEN;
};

struct ResourceLoader_Deflate_State {
  union {
    ResourceLoader_Deflate_State_T0 T0;
  };

  uint8_t *pOutStream;
  size_t OutStreamSize;

  size_t OutStreamOffset;

  EResourceLoader_Deflate_Stage Stage;
  uint32_t SubStage;

  uint8_t CompressionType;

  bool IsFinalChunk;
};

// Stream size is in bytes. It's likely that the end bit of the DEFLATE stream
// doesn't land on a byte boundary. That's fine as long as the byte stream
// contains the entire DEFLATE stream.
RESOURCE_LOADER_API int
ResourceLoader_deflate_read_partial(const uint8_t *pStream, size_t streamSize,
                                    ResourceLoader_Deflate_State *pState);