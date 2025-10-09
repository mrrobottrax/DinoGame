#pragma once

enum EResourceLoader_Deflate_Stage {
  RESOURCE_LOADER_DEFLATE_STAGE_INITIAL,
  RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL,
  RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE,

  RESOURCE_LOADER_DEFLATE_STAGE_T0_COPY_DATA,

  RESOURCE_LOADER_DEFLATE_STAGE_T1_SETUP_STATIC_TREE,

  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HLIT,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HDIST,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HCLEN,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_CL_ALPHABET,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS,

  RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE,
};

struct ResourceLoader_Deflate_State {
  struct HuffmanState {
    static const size_t k_MaxBits = 15;

    struct LengthData {
      uint16_t FirstCode;
      uint16_t FirstValue;
      uint8_t CodeCount;
    };

    struct {
      LengthData InfoForLength[k_MaxBits + 1];
      uint16_t NumberProvided;
      uint8_t LengthForAlphabet[19];
    } CodeLength;

    struct {
      LengthData InfoForLength[k_MaxBits + 1];
      uint16_t NumberProvided;
      uint8_t LengthForAlphabet[31];
    } Distance;

    struct {
      LengthData InfoForLength[k_MaxBits + 1];
      uint16_t NumberProvided;
      uint8_t LengthForAlphabet[287];
    } LiteralLength;
  };

  union {
    struct {
      uint16_t LEN;
      uint16_t NLEN;
    } T0;

    HuffmanState Huffman;
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