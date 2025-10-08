#include "pch.h"

#include "deflate.h"

#define DEFLATE_HEADER_UNSUPPORTED_COMPRESSION_TYPE MAKE_ERROR(00, 00, 00);
#define DEFLATE_T0_NLEN_MISMATCH MAKE_ERROR(00, 00, 01);
#define DEFLATE_UNSUPPORTED_STAGE MAKE_ERROR(00, 00, 02);
#define DEFLATE_CANNOT_UNCOMPRESS_T0 MAKE_ERROR(00, 00, 03);
#define DEFLATE_OUT_BUFFER_TOO_SMALL MAKE_ERROR(00, 00, 04);

static int deflate_state_machine(ResourceLoader_Deflate_State *pState,
                                 bool bit) {
  switch (pState->Stage) {
  case RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL:
    pState->SubStage = 0;
    pState->IsFinalChunk = bit;
    pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE;
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE:
    pState->CompressionType |= bit * (0b1 << pState->SubStage);
    ++pState->SubStage;

    if (pState->SubStage < 2)
      break;

    pState->SubStage = 0;

    ASSERT_RETURN(pState->CompressionType <= 2,
                  DEFLATE_HEADER_UNSUPPORTED_COMPRESSION_TYPE);

    if (pState->CompressionType == 0) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T0_COPY_DATA;
    } else if (pState->CompressionType == 1) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T1_SETUP_STATIC_TREE;
    } else if (pState->CompressionType == 2) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READ_HUFFMAN_TREE;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T1_SETUP_STATIC_TREE:
    return DEFLATE_UNSUPPORTED_STAGE;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READ_HUFFMAN_TREE:
    return DEFLATE_UNSUPPORTED_STAGE;

  case RESOURCE_LOADER_DEFLATE_STAGE_DECODE:
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
