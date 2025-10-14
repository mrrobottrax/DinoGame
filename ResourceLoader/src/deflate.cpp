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

#define DEFLATE_ZLIB_BAD_HEADER MAKE_ERROR(01, 00, 00);

typedef ResourceLoader_Deflate_State::HuffmanState HuffmanState;

static int
calc_tree(const uint16_t *pLengths, uint16_t *pTree, size_t nLengths,
          HuffmanState::LengthData pLengthInfo[HuffmanState::k_MaxBits + 1]) {
  // copy length info onto stack
  uint16_t *pLengthsNew = (uint16_t *)_malloca(sizeof(uint16_t) * nLengths);
  ASSERT_RETURN(pLengthsNew, DEFLATE_FAILED_ALLOCATION);
  memcpy(pLengthsNew, pLengths, sizeof(uint16_t) * nLengths);
  pLengths = pLengthsNew;

  // setup
  for (int i = 0; i <= HuffmanState::k_MaxBits; ++i) {
    pLengthInfo[i].CodeCount = 0;
    pLengthInfo[i].FirstCode = 0;
    pLengthInfo[i].FirstValueIndex = 0;
  }

  // get frequency of each length
  for (int i = 0; i < nLengths; ++i) {
    uint16_t length = pLengths[i];
    ASSERT_RETURN(length <= HuffmanState::k_MaxBits,
                  DEFLATE_CORRUPT_DATASTREAM);
    ++pLengthInfo[length].CodeCount;
  }

#ifdef DEBUG
  double sum = 0.0;
  for (size_t i = 1; i <= HuffmanState::k_MaxBits; ++i)
    sum += pLengthInfo[i].CodeCount / (double)(1u << i);

  ASSERT_RETURN(sum <= 1.0, DEFLATE_CORRUPT_DATASTREAM);
#endif

  pLengthInfo[0].CodeCount = 0;

  uint16_t next_index[HuffmanState::k_MaxBits + 1]{};

  // get starting code of each length
  uint16_t code = 0;
  uint16_t index = 0;
  for (int i = 1; i <= HuffmanState::k_MaxBits; ++i) {
    code = (code + pLengthInfo[i - 1].CodeCount) << 1;
    index = index + pLengthInfo[i - 1].CodeCount;
    ASSERT_RETURN(index <= nLengths, DEFLATE_CORRUPT_DATASTREAM);
    pLengthInfo[i].FirstCode = code;
    pLengthInfo[i].FirstValueIndex = index;
    next_index[i] = index;
  }

  // insert values into tree
  for (uint16_t i = 0; i < nLengths; ++i) {
    uint16_t length = pLengths[i];
    ASSERT_RETURN(length <= HuffmanState::k_MaxBits,
                  DEFLATE_CORRUPT_DATASTREAM);
    if (length > 0) {
      pTree[next_index[length]] = i;
      ++next_index[length];
    }
  }

  return 0;
}

static void setup_static_lengths(ResourceLoader_Deflate_State *pState) {
  for (uint16_t i = 0; i <= 143; ++i) {
    pState->Huffman.LiteralLength.Tree[i] = 8;
  }

  for (uint16_t i = 144; i <= 255; ++i) {
    pState->Huffman.LiteralLength.Tree[i] = 9;
  }

  for (uint16_t i = 256; i <= 279; ++i) {
    pState->Huffman.LiteralLength.Tree[i] = 7;
  }

  for (uint16_t i = 280; i <= 287; ++i) {
    pState->Huffman.LiteralLength.Tree[i] = 8;
  }

  pState->Huffman.LiteralLength.NumberProvided = 288;

  for (uint16_t i = 0; i <= 31; ++i) {
    pState->Huffman.Distance.Tree[i] = 5;
  }

  pState->Huffman.Distance.NumberProvided = 32;
}

static int bit_state_machine(ResourceLoader_Deflate_State *pState, bool bit) {
  switch (pState->Stage) {
  case RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL:
    pState->IsFinalChunk = bit;

    pState->SubStage = 0;
    pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE;
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BTYPE:
    pState->CompressionType |= bit << pState->SubStage;
    ++pState->SubStage;

    if (pState->SubStage < 2)
      break;

    ASSERT_RETURN(pState->CompressionType <= 2,
                  DEFLATE_HEADER_UNSUPPORTED_COMPRESSION_TYPE);

    pState->SubStage = 0;

    if (pState->CompressionType == 0) {
      pState->Uncompressed = {};
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T0_SKIP_BYTE;
    } else if (pState->CompressionType == 1) {
      pState->Huffman = {};
      setup_static_lengths(pState);
      PROPAGATE_CODE(calc_tree(pState->Huffman.Distance.Tree,
                               pState->Huffman.Distance.Tree,
                               _countof(pState->Huffman.Distance.Tree),
                               pState->Huffman.Distance.InfoForLength));
      PROPAGATE_CODE(calc_tree(pState->Huffman.LiteralLength.Tree,
                               pState->Huffman.LiteralLength.Tree,
                               _countof(pState->Huffman.LiteralLength.Tree),
                               pState->Huffman.LiteralLength.InfoForLength));
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE;
    } else if (pState->CompressionType == 2) {
      pState->Huffman = {};
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HLIT;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T0_READ_LEN_NLEN: {
    if (pState->SubStage < 8) {
      pState->Uncompressed.LEN |= bit << pState->SubStage;
    } else if (pState->SubStage < 16) {
      pState->Uncompressed.NLEN |= bit << pState->SubStage;
    }
    ++pState->SubStage;

    if (pState->SubStage < 16)
      break;

    ASSERT_RETURN(pState->Uncompressed.LEN ==
                      (pState->Uncompressed.NLEN ^ (uint16_t)~0),
                  DEFLATE_T0_NLEN_MISMATCH);

    pState->SubStage = 0;
    pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T0_COPY_DATA;
    break;
  }

  case RESOURCE_LOADER_DEFLATE_STAGE_T0_COPY_DATA:
    pState->Uncompressed.CurrentByte |= bit << pState->SubStage;
    ++pState->SubStage;

    if (pState->SubStage % 8 == 0) {
      uint8_t value = pState->Uncompressed.CurrentByte;
      pState->Uncompressed.CurrentByte = 0;

      pState->pOutStream[pState->OutStreamOffset] = value;
      ++pState->OutStreamOffset;

      ASSERT_RETURN(pState->OutStreamOffset <= pState->OutStreamSize,
                    DEFLATE_OUT_BUFFER_TOO_SMALL);

      uint32_t read = pState->SubStage / 8;
      if (read >= pState->Uncompressed.LEN) {
        pState->SubStage = 0;
        pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_END;
      }
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HLIT:
    pState->Huffman.LiteralLength.NumberProvided |= bit << pState->SubStage;
    ++pState->SubStage;

    if (pState->SubStage >= 5) {
      pState->Huffman.LiteralLength.NumberProvided += 257;
      ASSERT_RETURN(pState->Huffman.LiteralLength.NumberProvided <= 286,
                    DEFLATE_CORRUPT_DATASTREAM);

      pState->SubStage = 0;
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HDIST;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HDIST:
    pState->Huffman.Distance.NumberProvided |= bit << pState->SubStage;
    ++pState->SubStage;

    if (pState->SubStage >= 5) {
      pState->Huffman.Distance.NumberProvided += 1;
      ASSERT_RETURN(pState->Huffman.Distance.NumberProvided <=
                        _countof(pState->Huffman.Distance.Tree),
                    DEFLATE_CORRUPT_DATASTREAM);

      pState->SubStage = 0;
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HCLEN;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_HEADER_HCLEN:
    pState->Huffman.CodeLength.NumberProvided |= bit << pState->SubStage;
    ++pState->SubStage;

    if (pState->SubStage >= 4) {
      pState->Huffman.CodeLength.NumberProvided += 4;
      ASSERT_RETURN(pState->Huffman.CodeLength.NumberProvided <=
                        _countof(pState->Huffman.CodeLength.Tree),
                    DEFLATE_CORRUPT_DATASTREAM);

      pState->SubStage = 0;
      pState->Stage =
          RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_CL_ALPHABET;
    }
    break;

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_CL_ALPHABET: {
    uint32_t charIndex = pState->SubStage / 3;
    uint32_t bitIndex = pState->SubStage % 3;
    ++pState->SubStage;

    ASSERT_RETURN(charIndex < _countof(pState->Huffman.CodeLength.Tree),
                  DEFLATE_CORRUPT_DATASTREAM);

    constexpr uint8_t k_ClAlphabetOrder[] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                             11, 4,  12, 3, 13, 2, 14, 1, 15};

    charIndex = k_ClAlphabetOrder[charIndex];

    pState->Huffman.CodeLength.Tree[charIndex] |= bit << bitIndex;

    if (pState->SubStage < pState->Huffman.CodeLength.NumberProvided * 3u)
      break;

    // cheeky use of the same buffer for lengths and tree since we copy
    // lengths to the stack first
    PROPAGATE_CODE(calc_tree(pState->Huffman.CodeLength.Tree,
                             pState->Huffman.CodeLength.Tree,
                             _countof(pState->Huffman.CodeLength.Tree),
                             pState->Huffman.CodeLength.InfoForLength));

    pState->SubStage = 0;
    pState->Stage =
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_LITERAL_LENGTH_ALPHABET;
    break;
  }

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_LITERAL_LENGTH_ALPHABET:
  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET: {
    pState->Huffman.CurrentCode =
        (pState->Huffman.CurrentCode << 1) | bit * 0b1u;
    ++pState->Huffman.CurrentCodeLength;

    ASSERT_RETURN(pState->Huffman.CurrentCodeLength <= HuffmanState::k_MaxBits,
                  DEFLATE_CORRUPT_DATASTREAM);

    HuffmanState::LengthData &lengthData =
        pState->Huffman.CodeLength
            .InfoForLength[pState->Huffman.CurrentCodeLength];

    uint16_t firstCode = lengthData.FirstCode;
    uint16_t endCode = firstCode + lengthData.CodeCount;

    bool inRange = pState->Huffman.CurrentCode >= firstCode &&
                   pState->Huffman.CurrentCode < endCode;

    if (!inRange)
      break;

    uint16_t index = lengthData.FirstValueIndex;
    index += pState->Huffman.CurrentCode - firstCode;

    ASSERT_RETURN(index < _countof(pState->Huffman.CodeLength.Tree),
                  DEFLATE_CORRUPT_DATASTREAM);

    uint16_t value = pState->Huffman.CodeLength.Tree[index];

    ASSERT_RETURN(value < _countof(pState->Huffman.CodeLength.Tree),
                  DEFLATE_CORRUPT_DATASTREAM);

    uint16_t *tree = 0;
    uint16_t numProvided = 0;
    if (pState->Stage ==
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_LITERAL_LENGTH_ALPHABET) {
      tree = pState->Huffman.LiteralLength.Tree;
      numProvided = pState->Huffman.LiteralLength.NumberProvided;
    } else if (
        pState->Stage ==
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET) {
      tree = pState->Huffman.Distance.Tree;
      numProvided = pState->Huffman.Distance.NumberProvided;
    } else {
      ASSERT(false);
    }

    pState->Huffman.CurrentCode = 0;
    pState->Huffman.CurrentCodeLength = 0;
    pState->Huffman.CurrentValue0 = value;
    pState->Huffman.CurrentValue1 = 0;
    pState->Huffman.ExtraBitsValue0 = 0;
    pState->Huffman.ExtraBitsValue1 = 0;
    pState->Huffman.ReturnStage = pState->Stage;

    if (value <= 15) {
      tree[pState->SubStage] = value;
      ++pState->SubStage;
    } else if (value == 16) {
      pState->Stage =
          RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_COPY_EXTRA_BITS_FOR_CODE_LENGTHS;
    } else if (value == 17 || value == 18) {
      pState->Stage =
          RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_ZERO_EXTRA_BITS_FOR_CODE_LENGTHS;
    } else {
      ASSERT(false);
    }

    if (pState->SubStage < numProvided)
      break;

    pState->SubStage = 0;

    if (pState->Stage ==
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_LITERAL_LENGTH_ALPHABET) {
      pState->Stage =
          RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET;
    } else if (
        pState->Stage ==
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE;

      PROPAGATE_CODE(calc_tree(pState->Huffman.Distance.Tree,
                               pState->Huffman.Distance.Tree,
                               _countof(pState->Huffman.Distance.Tree),
                               pState->Huffman.Distance.InfoForLength));
      PROPAGATE_CODE(calc_tree(pState->Huffman.LiteralLength.Tree,
                               pState->Huffman.LiteralLength.Tree,
                               _countof(pState->Huffman.LiteralLength.Tree),
                               pState->Huffman.LiteralLength.InfoForLength));
    } else {
      ASSERT(false);
    }

    break;
  }

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_COPY_EXTRA_BITS_FOR_CODE_LENGTHS: {
    pState->Huffman.ExtraBitsValue0 |= bit << pState->Huffman.CurrentCodeLength;
    ++pState->Huffman.CurrentCodeLength;

    ASSERT(pState->Huffman.CurrentValue0 == 16);

    if (pState->Huffman.CurrentCodeLength < 2)
      break;

    uint16_t repeatAmt = pState->Huffman.ExtraBitsValue0 + 3;

    uint16_t *tree;
    uint16_t size;
    if (pState->Huffman.ReturnStage ==
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_LITERAL_LENGTH_ALPHABET) {
      tree = pState->Huffman.LiteralLength.Tree;
      size = _countof(pState->Huffman.LiteralLength.Tree);
    } else if (
        pState->Huffman.ReturnStage ==
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET) {
      tree = pState->Huffman.Distance.Tree;
      size = _countof(pState->Huffman.Distance.Tree);
    } else {
      ASSERT(false);
    }

    ASSERT_RETURN(pState->SubStage >= 1, DEFLATE_CORRUPT_DATASTREAM);
    uint16_t value = tree[pState->SubStage - 1];

    ASSERT_RETURN(pState->SubStage + repeatAmt <= size,
                  DEFLATE_CORRUPT_DATASTREAM);

    for (uint8_t i = 0; i < repeatAmt; ++i) {
      tree[pState->SubStage] = value;
      ++pState->SubStage;
    }

    pState->Huffman.CurrentCodeLength = 0;
    pState->Stage = pState->Huffman.ReturnStage;

    break;
  }

  case RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_ZERO_EXTRA_BITS_FOR_CODE_LENGTHS: {
    pState->Huffman.ExtraBitsValue0 |= bit << pState->Huffman.CurrentCodeLength;
    ++pState->Huffman.CurrentCodeLength;

    ASSERT(pState->Huffman.CurrentValue0 == 17 ||
           pState->Huffman.CurrentValue0 == 18);

    uint16_t extraBitsToRead = 0;
    if (pState->Huffman.CurrentValue0 == 17) {
      extraBitsToRead = 3;
    } else if (pState->Huffman.CurrentValue0 == 18) {
      extraBitsToRead = 7;
    }

    if (pState->Huffman.CurrentCodeLength < extraBitsToRead)
      break;

    uint16_t amtToFill = 0;
    if (pState->Huffman.CurrentValue0 == 17) {
      amtToFill = 3 + pState->Huffman.ExtraBitsValue0;
    } else if (pState->Huffman.CurrentValue0 == 18) {
      amtToFill = 11 + pState->Huffman.ExtraBitsValue0;
    }

    uint16_t *tree;
    uint16_t size;
    if (pState->Huffman.ReturnStage ==
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_LITERAL_LENGTH_ALPHABET) {
      tree = pState->Huffman.LiteralLength.Tree;
      size = _countof(pState->Huffman.LiteralLength.Tree);
    } else if (
        pState->Huffman.ReturnStage ==
        RESOURCE_LOADER_DEFLATE_STAGE_T2_READING_CODE_LENGTHS_FOR_DISTANCE_ALPHABET) {
      tree = pState->Huffman.Distance.Tree;
      size = _countof(pState->Huffman.Distance.Tree);
    } else {
      ASSERT(false);
    }

    ASSERT_RETURN(pState->SubStage + amtToFill <= size,
                  DEFLATE_CORRUPT_DATASTREAM);

    for (uint16_t i = 0; i < amtToFill; ++i) {
      tree[pState->SubStage] = 0;
      ++pState->SubStage;
    }

    pState->Huffman.CurrentCodeLength = 0;
    pState->Stage = pState->Huffman.ReturnStage;

    break;
  }

  case RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE: {
    ASSERT_RETURN(pState->CompressionType != 0, DEFLATE_CANNOT_UNCOMPRESS_T0);

    pState->Huffman.CurrentCode =
        (pState->Huffman.CurrentCode << 1) | bit * 0b1u;
    ++pState->Huffman.CurrentCodeLength;

    ASSERT_RETURN(pState->Huffman.CurrentCodeLength <= HuffmanState::k_MaxBits,
                  DEFLATE_CORRUPT_DATASTREAM);

    HuffmanState::LengthData &lengthInfo =
        pState->Huffman.LiteralLength
            .InfoForLength[pState->Huffman.CurrentCodeLength];

    uint16_t firstCode = lengthInfo.FirstCode;
    uint16_t endCode = lengthInfo.FirstCode + lengthInfo.CodeCount;

    bool inRange = pState->Huffman.CurrentCode >= firstCode &&
                   pState->Huffman.CurrentCode < endCode;

    if (!inRange)
      break;

    uint16_t index =
        lengthInfo.FirstValueIndex + pState->Huffman.CurrentCode - firstCode;

    ASSERT_RETURN(index < _countof(pState->Huffman.LiteralLength.Tree),
                  DEFLATE_CORRUPT_DATASTREAM);

    uint16_t value = pState->Huffman.LiteralLength.Tree[index];

    ASSERT_RETURN(value <= 285, DEFLATE_CORRUPT_DATASTREAM);

    pState->Huffman.CurrentCode = 0;
    pState->Huffman.CurrentCodeLength = 0;
    pState->Huffman.CurrentValue0 = value;
    pState->Huffman.CurrentValue1 = 0;
    pState->Huffman.ExtraBitsValue0 = 0;
    pState->Huffman.ExtraBitsValue1 = 0;
    pState->Huffman.ReturnStage = pState->Stage;
    pState->SubStage = 0;

    if (value <= 255) {
      pState->pOutStream[pState->OutStreamOffset] = (uint8_t)value;
      ++pState->OutStreamOffset;

      ASSERT_RETURN(pState->OutStreamOffset <= pState->OutStreamSize,
                    DEFLATE_OUT_BUFFER_TOO_SMALL);
    } else if (value == 256) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_END;
    } else if (value >= 257) {
      pState->Stage =
          RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_LITERAL_LENGTH_EXTRA_BITS;
      if (pState->Huffman.CurrentValue0 < 265 ||
          pState->Huffman.CurrentValue0 > 284) {
        // extraBitCount == 0, don't read next bit
        pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE;
      }
    }
    break;
  }

  case RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_LITERAL_LENGTH_EXTRA_BITS: {
    uint16_t extraBitsCount = 0;
    if (pState->Huffman.CurrentValue0 >= 265 &&
        pState->Huffman.CurrentValue0 <= 284) {
      extraBitsCount = (pState->Huffman.CurrentValue0 - 261) / 4;
    }

    if (extraBitsCount) {
      pState->Huffman.ExtraBitsValue0 = bit << pState->SubStage;
      ++pState->SubStage;
    }

    if (pState->SubStage >= extraBitsCount) {
      pState->SubStage = 0;
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE;
    }

    break;
  }

  case RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE: {
    ASSERT_RETURN(pState->CompressionType != 0, DEFLATE_CANNOT_UNCOMPRESS_T0);

    pState->Huffman.CurrentCode =
        (pState->Huffman.CurrentCode << 1) | bit * 0b1u;
    ++pState->Huffman.CurrentCodeLength;

    HuffmanState::LengthData &lengthInfo =
        pState->Huffman.Distance
            .InfoForLength[pState->Huffman.CurrentCodeLength];

    uint16_t firstCode = lengthInfo.FirstCode;
    uint16_t endCode = lengthInfo.FirstCode + lengthInfo.CodeCount;

    bool inRange = pState->Huffman.CurrentCode >= firstCode &&
                   pState->Huffman.CurrentCode < endCode;

    if (!inRange)
      break;

    uint16_t index =
        lengthInfo.FirstValueIndex + pState->Huffman.CurrentCode - firstCode;
    uint16_t value = pState->Huffman.Distance.Tree[index];

    ASSERT_RETURN(value <= 29, DEFLATE_CORRUPT_DATASTREAM);

    pState->Huffman.CurrentValue1 = value;

    pState->SubStage = 0;
    pState->Stage =
        RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE_EXTRA_BITS;

    if (pState->Huffman.CurrentValue1 < 4 ||
        pState->Huffman.CurrentValue1 > 29) {
      // extraBitCount == 0, don't read next bit
      goto RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE_EXTRA_BITS;
    }
    break;
  }

  RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE_EXTRA_BITS:
  case RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE_DISTANCE_EXTRA_BITS: {
    uint16_t extraBitsCount = 0;
    if (pState->Huffman.CurrentValue1 >= 4 &&
        pState->Huffman.CurrentValue1 <= 29) {
      extraBitsCount = (pState->Huffman.CurrentValue1 - 2) / 2;
    }

    if (extraBitsCount) {
      pState->Huffman.ExtraBitsValue1 = bit << pState->SubStage;
      ++pState->SubStage;
    }

    if (pState->SubStage < extraBitsCount)
      break;

    uint16_t bucket;
    uint16_t bucketStartingCode;
    uint16_t indexInBucket;
    uint16_t incrementSize;

    // length
    constexpr uint16_t lengthBuckets[] = {0, 11, 19, 35, 67, 131, 258};

    uint16_t length = 0;
    if (pState->Huffman.CurrentValue0 >= 265 &&
        pState->Huffman.CurrentValue0 <= 285) {
      bucket = (pState->Huffman.CurrentValue0 - 261) / 4;
      bucketStartingCode = bucket * 4 + 261;
    } else {
      bucket = 0;
      bucketStartingCode = 257;
    }

    indexInBucket = pState->Huffman.CurrentValue0 - bucketStartingCode;
    incrementSize = 1 << bucket;

    ASSERT(bucket < _countof(lengthBuckets));
    length = lengthBuckets[bucket] + pState->Huffman.ExtraBitsValue0 +
             incrementSize * indexInBucket;

    // distance
    constexpr uint16_t distanceBuckets[] = {
        1, 5, 9, 17, 33, 65, 129, 257, 513, 1025, 2049, 4097, 8193, 16385};

    uint16_t distance = 0;
    if (pState->Huffman.CurrentValue1 >= 4 &&
        pState->Huffman.CurrentValue1 <= 29) {
      bucket = (pState->Huffman.CurrentValue1 - 2) / 2;
      bucketStartingCode = bucket * 2 + 2;
    } else {
      bucket = 0;
      bucketStartingCode = 0;
    }

    indexInBucket = pState->Huffman.CurrentValue1 - bucketStartingCode;
    incrementSize = 1 << bucket;

    ASSERT(bucket < _countof(distanceBuckets));
    distance = distanceBuckets[bucket] + pState->Huffman.ExtraBitsValue1 +
               incrementSize * indexInBucket;

    ASSERT_RETURN(pState->OutStreamOffset >= distance,
                  DEFLATE_CORRUPT_DATASTREAM);

    ASSERT_RETURN(pState->OutStreamOffset + length <= pState->OutStreamSize,
                  DEFLATE_OUT_BUFFER_TOO_SMALL);

    size_t start = pState->OutStreamOffset - distance;
    for (uint16_t i = 0; i < length; ++i) {
      pState->pOutStream[pState->OutStreamOffset++] = pState->pOutStream[start];
      ++start;
    }

    pState->Huffman.CurrentCode = 0;
    pState->Huffman.CurrentCodeLength = 0;
    pState->Huffman.CurrentValue0 = 0;
    pState->Huffman.CurrentValue1 = 0;
    pState->Huffman.ExtraBitsValue0 = 0;
    pState->Huffman.ExtraBitsValue1 = 0;
    pState->SubStage = 0;

    pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_HUFFMAN_DECODE;

    break;
  }

  default:
    return DEFLATE_UNSUPPORTED_STAGE;
  }

  return 0;
}

RESOURCE_LOADER_API int
ResourceLoader_deflate_read_partial(const uint8_t *pStream, size_t streamSize,
                                    ResourceLoader_Deflate_State *pState) {
  if (pState->Stage == RESOURCE_LOADER_DEFLATE_STAGE_INITIAL) {
    pState->SubStage = 0;
    if (pState->NoZlib) {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL;
    } else {
      pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_ZLIB_HEADER;
    }
  }

  size_t byte = 0;

  if (pState->Stage == RESOURCE_LOADER_DEFLATE_STAGE_ZLIB_HEADER) {
    for (; byte < streamSize; ++byte) {
      uint8_t b = *(pStream + byte);

      if (byte == 0) {
        pState->ZlibHeader.Method = 0b00001111 & b;
        pState->ZlibHeader.Info = (0b11110000 & b) >> 4;

        ASSERT_RETURN(pState->ZlibHeader.Method == 8, DEFLATE_ZLIB_BAD_HEADER);
      } else if (byte == 1) {
        pState->ZlibHeader.UsePresetDict = 0b100000 & b;
        pState->ZlibHeader.Level = (0b11000000 & b) >> 6;

        uint8_t cmf =
            pState->ZlibHeader.Method + (pState->ZlibHeader.Info << 4);
        uint8_t flg = b;
        uint16_t check = cmf * 256 + flg;

        ASSERT_RETURN(check % 31 == 0, DEFLATE_ZLIB_BAD_HEADER);

        if (!pState->ZlibHeader.UsePresetDict) {
          pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL;
          break;
        }
      } else if (byte >= 2 && byte <= 5) {
        pState->ZlibHeader.DictId = (pState->ZlibHeader.DictId << 8) | b;
      } else {
        pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL;
        break;
      }
    }
    ++byte;
  }

  if (pState->Stage != RESOURCE_LOADER_DEFLATE_STAGE_ZLIB_CHECK_ADLER) {
    for (; byte < streamSize; ++byte) {
      for (uint8_t bit = 0; bit < 8; ++bit) {
        bool set = *(pStream + byte) & (1 << bit);

        int result = bit_state_machine(pState, set);
        if (result != 0) {
          return result;
        }

        if (pState->Stage == RESOURCE_LOADER_DEFLATE_STAGE_END) {
          pState->SubStage = 0;

          if (pState->IsFinalChunk) {
            if (!pState->NoZlib) {
              pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_ZLIB_CHECK_ADLER;
              break;
            }

            return 0;
          } else {
            pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_READING_HEADER_BFINAL;
          }
        } else if (pState->Stage ==
                   RESOURCE_LOADER_DEFLATE_STAGE_T0_SKIP_BYTE) {
          pState->Stage = RESOURCE_LOADER_DEFLATE_STAGE_T0_READ_LEN_NLEN;
          break;
        }
      }

      if (pState->Stage == RESOURCE_LOADER_DEFLATE_STAGE_ZLIB_CHECK_ADLER) {
        ++byte;
        break;
      }
    }
  }

  if (pState->Stage == RESOURCE_LOADER_DEFLATE_STAGE_ZLIB_CHECK_ADLER) {
    for (; byte < streamSize; ++byte) {
      uint8_t b = *(pStream + byte);
      ++pState->SubStage;

      pState->ZlibHeader.Adler = (pState->ZlibHeader.Adler << 8) | b;

      if (pState->SubStage >= 4) {
        // TODO: check adler
        return 0;
      }
    }
  }

  return 0;
}
