#include "pch.h"

#include "deflate.h"

#define DEFLATE_HEADER_NO_SPACE MAKE_ERROR(00, 00, 00);
#define DEFLATE_HEADER_UNSUPPORTED_COMPRESSION_TYPE MAKE_ERROR(00, 00, 01);

// static bool read_bit(uint8_t const *pStream, size_t streamSize,
//                      size_t *pBitOffset) {
//   size_t byte = *pBitOffset / 8;
//   uint8_t localOffset = *pBitOffset % 8;
//
//   *pBitOffset++;
//
//   pStream += byte;
//   streamSize -= byte;
//
//   bool hasNextBit = *pStream
// }
//
// static int stage_INITIAL(const uint8_t *pStream, size_t streamSize,
//                          size_t *pBitOffset,
//                          ResourceLoader_Deflate_State *pState) {
//   ASSERT_RETURN(*pBitOffset / 8 < streamSize, DEFLATE_HEADER_NO_SPACE);
//   pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READ_HEADER;
// }
//
// static int stage_READ_HEADER(const uint8_t *pStream, size_t streamSize,
//                              size_t *pBitOffset,
//                              ResourceLoader_Deflate_State *pState) {
//   pState->IsFinalChunk = read_bit(pStream, streamSize, pBitOffset);
//
//   for (int i = 0; i < 2; ++i) {
//     uint8_t bitOffset = *pBitOffset % 8;
//     if (bitOffset == 0) {
//       ++pStream;
//       --streamSize;
//       ASSERT_RETURN(streamSize > 0, DEFLATE_HEADER_NO_SPACE);
//     }
//
//     bool bit = *pStream & (0b1 << bitOffset);
//     *pBitOffset++;
//
//     uint8_t mask = bit ? 0b1 : 0b0;
//     pState->CompressionType |= mask << i;
//   }
//
//   ASSERT_RETURN(pState->CompressionType == 0 || pState->CompressionType == 1
//   ||
//                     pState->CompressionType == 2,
//                 DEFLATE_HEADER_UNSUPPORTED_COMPRESSION_TYPE);
//
//   *pBitsRead = 3;
//
//   if (pState->CompressionType == 0) {
//     *pBitsRead = 8;
//   }
//
//   pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READ_HEADER;
// }

static int deflate_state_machine(ResourceLoader_Deflate_State *pState,
                                 bool bit) {
  return 1;
}

RESOURCE_LOADER_API int
ResourceLoader_deflate_read_partial(const uint8_t *pStream, size_t streamSize,
                                    ResourceLoader_Deflate_State *pState) {
  for (size_t byte = 0; byte < streamSize; ++byte) {
    for (uint8_t bit = 0; bit < 8; ++bit) {
      bool set = *(pStream + byte) & (1 << bit);

      int result = deflate_state_machine(pState, set);
      if (result != 0) {
        return result;
      }
    }
  }

  return 0;
}
