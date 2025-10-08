#include "pch.h"

#include "deflate.h"

#define DEFLATE_HEADER_UNSUPPORTED_COMPRESSION_TYPE MAKE_ERROR(00, 00, 00);
#define DEFLATE_T0_NLEN_MISMATCH MAKE_ERROR(00, 00, 01);
#define DEFLATE_UNSUPPORTED_STAGE MAKE_ERROR(00, 00, 02);
#define DEFLATE_CANNOT_UNCOMPRESS_T0 MAKE_ERROR(00, 00, 03);
#define DEFLATE_OUT_BUFFER_TOO_SMALL MAKE_ERROR(00, 00, 04);
#define DEFLATE_CORRUPT_DATASTREAM MAKE_ERROR(00, 00, 05);
#define DEFLATE_NOT_ENOUGH_MEMORY MAKE_ERROR(00, 00, 06);
#define DEFLATE_FAILED_ALLOCATION MAKE_ERROR(00, 00, 07);

typedef ResourceLoader_Deflate_State::Node Node;

static int construct_tree(const uint8_t *pLengths, size_t nLengths, Node *pTree,
                          size_t treeArrayLength) {
  ASSERT_RETURN(nLengths <= treeArrayLength, DEFLATE_CORRUPT_DATASTREAM);

  uint8_t *pNewLengthsBuffer = (uint8_t *)_malloca(sizeof(uint8_t) * nLengths);
  ASSERT_RETURN(pNewLengthsBuffer, DEFLATE_FAILED_ALLOCATION);

  memcpy_s(pNewLengthsBuffer, sizeof(uint8_t) * nLengths, pLengths,
           sizeof(uint8_t) * nLengths);

  pLengths = pNewLengthsBuffer;

  uint8_t counts[15]{};

  for (size_t i = 0; i < nLengths; ++i) {
    uint8_t length = pLengths[i];
    ASSERT_RETURN(length <= 14, DEFLATE_NOT_ENOUGH_MEMORY);
    ++counts[length];
  }

  return 1;
}

static int deflate_state_machine(ResourceLoader_Deflate_State *pState,
                                 bool bit) {
  switch (pState->Stage) {
  case RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL:
    pState->IsFinalChunk = bit;
    pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE;
    pState->SubStage = 0;
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE:
    pState->CompressionType |= bit * (0b1 << pState->SubStage);
    ++pState->SubStage;

    if (pState->SubStage < 2)
      break;

    ASSERT_RETURN(pState->CompressionType <= 2,
                  DEFLATE_HEADER_UNSUPPORTED_COMPRESSION_TYPE);

    pState->SubStage = 0;
    if (pState->CompressionType == 0) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T0_COPY_DATA;
    } else if (pState->CompressionType == 1) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T1_SETUP_STATIC_TREE;
    } else if (pState->CompressionType == 2) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HLIT;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T1_SETUP_STATIC_TREE:
    return DEFLATE_UNSUPPORTED_STAGE;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HLIT:
    pState->Huffman.NumberOfLiteralLengthCodes |=
        bit * (0b1 << pState->SubStage);
    ++pState->SubStage;

    if (pState->SubStage >= 5) {
      pState->Huffman.NumberOfLiteralLengthCodes += 257;
      ASSERT_RETURN(pState->Huffman.NumberOfLiteralLengthCodes <= 286,
                    DEFLATE_CORRUPT_DATASTREAM);
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HDIST;
      pState->SubStage = 0;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HDIST:
    pState->Huffman.NumberOfDistanceCodes |= bit * (0b1 << pState->SubStage);
    ++pState->SubStage;

    if (pState->SubStage >= 5) {
      pState->Huffman.NumberOfDistanceCodes += 1;
      ASSERT_RETURN(pState->Huffman.NumberOfDistanceCodes <= 32,
                    DEFLATE_CORRUPT_DATASTREAM);
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HCLEN;
      pState->SubStage = 0;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HCLEN:
    pState->Huffman.NumberofCodeLengthCodes |= bit * (0b1 << pState->SubStage);
    ++pState->SubStage;

    if (pState->SubStage >= 4) {
      pState->Huffman.NumberofCodeLengthCodes += 4;
      ASSERT_RETURN(pState->Huffman.NumberofCodeLengthCodes <= 19,
                    DEFLATE_CORRUPT_DATASTREAM);
      pState->Stage =
          RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_CL_ALPHABET;
      pState->SubStage = 0;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_CL_ALPHABET: {
    uint32_t charIndex = pState->SubStage / 3;
    uint32_t bitIndex = pState->SubStage % 3;
    ++pState->SubStage;

    ASSERT_RETURN(charIndex < 19, DEFLATE_CORRUPT_DATASTREAM);

    constexpr uint8_t k_ClAlphabetOrder[] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                             11, 4,  12, 3, 13, 2, 14, 1, 15};

    charIndex = k_ClAlphabetOrder[charIndex];

    pState->Huffman.CodeLengthLengths[charIndex] |= bit * (0b1 << bitIndex);

    if (pState->SubStage >= pState->Huffman.NumberofCodeLengthCodes * 3u) {
      int code = construct_tree(pState->Huffman.CodeLengthLengths,
                                pState->Huffman.NumberofCodeLengthCodes,
                                pState->Huffman.CodeLengthTree,
                                _countof(pState->Huffman.CodeLengthTree));

      if (code != 0)
        return code;

      pState->Stage =
          RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET;
      pState->SubStage = 0;
    }
    break;
  }

  case RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE:
    ASSERT_RETURN(pState->CompressionType != 0, DEFLATE_CANNOT_UNCOMPRESS_T0);

    return DEFLATE_UNSUPPORTED_STAGE;

  default:
    return DEFLATE_UNSUPPORTED_STAGE;
  }

  return 0;
}

RESOURCE_LOADER_API int
ResourceLoader_deflate_read_partial(const uint8_t *pStream, size_t streamSize,
                                    ResourceLoader_Deflate_State *pState) {
  if (pState->Stage == RESOURCE_LOADER_DEFLATE_STAGE_INITIAL)
    pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL;

  for (size_t byte = 0; byte < streamSize; ++byte) {
    for (uint8_t bit = 0; bit < 8; ++bit) {
      bool set = *(pStream + byte) & (1 << bit);

      int result = deflate_state_machine(pState, set);
      if (result != 0) {
        return result;
      }

      if (pState->Stage == RESOURCE_LOADER_DEFLATE_STAGE_T0_COPY_DATA) {
        ++byte;
        bit = 0;

        pState->T0.LEN = 0;
        pState->T0.NLEN = 0;
        pState->T0.LEN |= *(uint16_t *)(pStream + byte);
        pState->T0.NLEN |= *(uint16_t *)(pStream + byte + 2);

        ASSERT_RETURN(pState->T0.LEN == (pState->T0.NLEN ^ (uint16_t)~0),
                      DEFLATE_T0_NLEN_MISMATCH);

        pStream = pStream + byte + 4;

        ASSERT_RETURN(pState->OutStreamSize - pState->OutStreamOffset >=
                          pState->T0.LEN,
                      DEFLATE_OUT_BUFFER_TOO_SMALL);

        for (byte = 0; byte < pState->T0.LEN; ++byte) {
          uint8_t v = pStream[byte];
          pState->pOutStream[pState->OutStreamOffset] = v;
        }

        return 0;
      }
    }
  }

  return 0;
}
