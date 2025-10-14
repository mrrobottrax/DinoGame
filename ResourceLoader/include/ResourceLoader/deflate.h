#pragma once

enum EResourceLoader_Deflate_Stage {
  RESOURCE_LOADER_DEFLATE_STAGE_INITIAL,

  RESOURCE_LOADER_DEFLATE_STAGE_ZLIB_HEADER,

  RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL,
  RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE,

  RESOURCE_LOADER_DEFLATE_STAGE_T0_SKIP_BYTE,
  RESOURCE_LOADER_DEFLATE_STAGE_T0_READ_LEN_NLEN,
  RESOURCE_LOADER_DEFLATE_STAGE_T0_COPY_DATA,

  RESOURCE_LOADER_DEFLATE_STAGE_T1_SETUP_STATIC_TREE,

  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HLIT,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HDIST,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HCLEN,

  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_CL_ALPHABET,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_LITERAL_LENGTH_ALPHABET,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET,

  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_COPY_EXTRA_BITS_FOR_CODE_LENGTHS,
  RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_ZERO_EXTRA_BITS_FOR_CODE_LENGTHS,

  RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE,
  RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_LITERAL_LENGTH_EXTRA_BITS,
  RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE,
  RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE_EXTRA_BITS,

  RESOURCE_LOADER_DEFLATE_STAGE_END,

  RESOURCE_LOADER_DEFLATE_STAGE_ZLIB_CHECK_ADLER,
};

struct ResourceLoader_Deflate_State {
  struct HuffmanState {
    static const size_t k_MaxBits = 15;

    struct LengthData {
      uint16_t FirstCode;
      uint16_t FirstValueIndex;
      uint8_t CodeCount;
    };

    struct {
      LengthData InfoForLength[k_MaxBits + 1];
      uint16_t NumberProvided;
      uint16_t Tree[19];
    } CodeLength;

    struct {
      LengthData InfoForLength[k_MaxBits + 1];
      uint16_t NumberProvided;
      uint16_t Tree[288];
    } LiteralLength;

    struct {
      LengthData InfoForLength[k_MaxBits + 1];
      uint16_t NumberProvided;
      uint16_t Tree[32];
    } Distance;

    EResourceLoader_Deflate_Stage ReturnStage;

    uint16_t CurrentCode;

    // Value for code length or literal/length
    uint16_t CurrentValue0;
    // Value for backwards distance
    uint16_t CurrentValue1;

    // Extra bits for code length or literal/length
    uint16_t ExtraBitsValue0;
    // Extra bits for backwards distance
    uint16_t ExtraBitsValue1;

    uint8_t CurrentCodeLength;
  };

  uint8_t *pOutStream;
  size_t OutStreamSize;

  size_t OutStreamOffset;

  union {
    struct {
      uint16_t LEN;
      uint16_t NLEN;

      uint8_t CurrentByte;
    } Uncompressed;

    HuffmanState Huffman;
  };

  struct {
    uint32_t DictId;
    uint32_t Adler;

    uint8_t Method;
    uint8_t Info;
    uint8_t Level;

    bool UsePresetDict;
  } ZlibHeader;

  EResourceLoader_Deflate_Stage Stage;
  uint32_t SubStage;

  uint8_t CompressionType;

  bool IsFinalChunk;

  // Stream has no Zlib header
  bool NoZlib;
};

// Stream size is in bytes. It's likely that the end bit of the DEFLATE stream
// doesn't land on a byte boundary. That's fine as long as the byte stream
// contains the entire DEFLATE stream.
RESOURCE_LOADER_API int
ResourceLoader_deflate_read_partial(const uint8_t *pStream, size_t streamSize,
                                    ResourceLoader_Deflate_State *pState);