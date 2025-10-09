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

typedef ResourceLoader_Deflate_State::HuffmanState HuffmanState;

static int get_initial_codes(const uint8_t *pAlphabetLengths,
                             size_t nAlphabetLengths,
                             HuffmanState::LengthData *pLengthInfo) {
  for (int i = 0; i <= HuffmanState::k_MaxBits; ++i) {
    pLengthInfo[i].CodeCount = 0;
    pLengthInfo[i].FirstCode = ~0u;
    pLengthInfo[i].FirstValue = ~0u;
  }

  // get frequency of each length
  for (int i = 0; i < nAlphabetLengths; ++i) {
    uint8_t length = pAlphabetLengths[i];
    ASSERT_RETURN(length <= HuffmanState::k_MaxBits, DEFLATE_NOT_ENOUGH_MEMORY);
    ++pLengthInfo[length].CodeCount;
    if (i < pLengthInfo[length].FirstValue)
      pLengthInfo[length].FirstValue = i;
  }

  // get starting code of each length
  pLengthInfo[0].CodeCount = 0;
  pLengthInfo[0].FirstCode = 0;
  pLengthInfo[0].FirstValue = 0;
  uint16_t code = 0;
  for (int i = 1; i <= HuffmanState::k_MaxBits; ++i) {
    code = (code + pLengthInfo[i - 1].CodeCount) << 1;

    if (pLengthInfo[i].CodeCount == 0)
      continue;

    pLengthInfo[i].FirstCode = code;
  }

  return 0;
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
    pState->Huffman.LiteralLength.NumberProvided |=
        bit * (0b1 << pState->SubStage);
    ++pState->SubStage;

    if (pState->SubStage >= 5) {
      pState->Huffman.LiteralLength.NumberProvided += 257;
      ASSERT_RETURN(pState->Huffman.LiteralLength.NumberProvided <= 286,
                    DEFLATE_CORRUPT_DATASTREAM);
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HDIST;
      pState->SubStage = 0;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HDIST:
    pState->Huffman.Distance.NumberProvided |= bit * (0b1 << pState->SubStage);
    ++pState->SubStage;

    if (pState->SubStage >= 5) {
      pState->Huffman.Distance.NumberProvided += 1;
      ASSERT_RETURN(pState->Huffman.Distance.NumberProvided <= 32,
                    DEFLATE_CORRUPT_DATASTREAM);
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HCLEN;
      pState->SubStage = 0;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HCLEN:
    pState->Huffman.CodeLength.NumberProvided |=
        bit * (0b1 << pState->SubStage);
    ++pState->SubStage;

    if (pState->SubStage >= 4) {
      pState->Huffman.CodeLength.NumberProvided += 4;
      ASSERT_RETURN(pState->Huffman.CodeLength.NumberProvided <= 19,
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

    pState->Huffman.CodeLength.LengthForAlphabet[charIndex] |=
        bit * (0b1 << bitIndex);

    if (pState->SubStage >= pState->Huffman.CodeLength.NumberProvided * 3u) {
      int code = get_initial_codes(pState->Huffman.CodeLength.LengthForAlphabet,
                                   pState->Huffman.CodeLength.NumberProvided,
                                   pState->Huffman.CodeLength.InfoForLength);

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
