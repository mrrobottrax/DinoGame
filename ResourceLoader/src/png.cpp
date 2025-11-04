#include "pch.h"

#include "arenas_private.h"
#include "deflate.h"
#include "png.h"

#define PNG_OUT_OF_MEMORY MAKE_ERROR(00, 00, 00)

#define PNG_BAD_MAGIC_NUMBER MAKE_ERROR(01, 00, 00)
#define PNG_BAD_CRC MAKE_ERROR(01, 00, 01)
#define PNG_BAD_CHUNK_ORDERING MAKE_ERROR(01, 00, 02)

#define PNG_UNSUPPORTED_CHUNK_NAME MAKE_ERROR(01, 01, 00)

#define PNG_TOO_SMALL_FOR_HEADER MAKE_ERROR(02, 00, 00)
#define PNG_TOO_SMALL_FOR_CHUNK MAKE_ERROR(02, 00, 01)
#define PNG_TOO_SMALL_FOR_CHUNK_HEADER MAKE_ERROR(02, 00, 02)
#define PNG_TOO_SMALL_FOR_CHUNK_LENGTH MAKE_ERROR(02, 00, 03)
#define PNG_TOO_SMALL_FOR_MAGIC_NUMBER MAKE_ERROR(02, 00, 04)

#define PNG_IHDR_INVALID_BIT_DEPTH MAKE_ERROR(03, 00, 00)
#define PNG_IHDR_INVALID_DIMENSIONS MAKE_ERROR(03, 00, 01)

#define PNG_IHDR_UNSUPPORTED_COMPRESSION_METHOD MAKE_ERROR(03, 01, 00)
#define PNG_IHDR_UNSUPPORTED_INTERLACE_METHOD MAKE_ERROR(03, 01, 01)
#define PNG_IHDR_UNSUPPORTED_FILTER_METHOD MAKE_ERROR(03, 01, 02)
#define PNG_IHDR_UNSUPPORTED_COLOR_TYPE MAKE_ERROR(03, 01, 03)

#define PNG_PLTE_INVALID_COLOR_TYPE MAKE_ERROR(04, 00, 00)
#define PNG_PLTE_INVALID_SIZE MAKE_ERROR(04, 00, 01)

#define PNG_IDAT_DEFLATE_ERROR MAKE_ERROR(05, 00, 00)

#define PNG_IEND_UNSUPPORTED_FILTER_METHOD MAKE_ERROR(06, 00, 00)

#define PNG_DEINTERLACE_BAD_INPUT MAKE_ERROR(07, 00, 00)

#define PNG_CONVERT_UNSUPPORTED_FORMAT MAKE_ERROR(08, 00, 00)
#define PNG_CONVERT_UNSUPPORTED_INTERLACE MAKE_ERROR(08, 00, 01)

static uint32_t s_CrcTable[256];
static bool s_CrcTableComputed;

enum EStage {
  STAGE_INITIAL,
  STAGE_READ_HEADER,
  STAGE_READ_PALETTE,
  STAGE_READ_DATA,
  STAGE_END,
};

struct State {
  ResourceLoader_Deflate_State DeflateState;

  uint8_t *Data;
  const uint8_t *Palette;
  size_t Size;
  ResourceLoader_arena_t Arena;
  uint32_t Width;
  uint32_t Height;
  uint32_t PaletteCount;
  EStage Stage;
  uint8_t BitDepth;
  uint8_t ColorType;
  uint8_t CompressionMethod;
  uint8_t FilterMethod;
  uint8_t InterlaceMethod;
};

constexpr static uint32_t encode(const char name[4]) {
  uint32_t val = 0;

  for (int i = 0; i < 4; ++i) {
    val <<= 8;
    val |= name[i];
  }

  return val;
}

static void make_crc_table() {
  uint32_t c;
  int n, k;

  for (n = 0; n < 256; ++n) {
    c = (unsigned int)n;
    for (k = 0; k < 8; ++k) {
      if (c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    s_CrcTable[n] = c;
  }
}

static uint32_t update_crc(uint32_t crc, const uint8_t *pBuffer, size_t len) {
  unsigned long c = crc;
  size_t n;

  if (!s_CrcTableComputed)
    make_crc_table();
  for (n = 0; n < len; n++) {
    c = s_CrcTable[(c ^ pBuffer[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

static uint32_t calc_crc(const uint8_t *pBuffer, size_t len) {
  return update_crc(0xffffffff, pBuffer, len) ^ 0xffffffff;
}

static uint32_t png_u32(const uint8_t *pValue) {
  union {
    uint32_t v = 0;
    uint8_t p1[4];
  } u;

  ENDIAN_TODO
  u.p1[0] = pValue[3];
  u.p1[1] = pValue[2];
  u.p1[2] = pValue[1];
  u.p1[3] = pValue[0];

  return u.v;
}

static uint8_t png_u8(const uint8_t *pValue) { return *pValue; }

#define ASSERT_CHUNK_ORDER(afterOrDuring, before)                              \
  ASSERT_RETURN(state.Stage >= afterOrDuring && state.Stage < before,          \
                PNG_BAD_CHUNK_ORDERING)

static uint32_t get_colors(uint32_t colorType) {
  uint32_t colors = 0;
  switch (colorType) {
  case 0:
    colors = 1;
    break;
  case 2:
    colors = 3;
    break;
  case 3:
    colors = 1;
    break;
  case 4:
    colors = 2;
    break;
  case 6:
    colors = 4;
    break;
  }
  ASSERT(colors != 0);

  return colors;
}

static void get_pass_widths(size_t width, size_t widths[7]) {
  widths[0] = (width + 7) / 8;
  widths[1] = (width + 3) / 8;
  widths[2] = (width + 3) / 4;
  widths[3] = (width + 1) / 4;
  widths[4] = (width + 1) / 2;
  widths[5] = (width + 0) / 2;
  widths[6] = (width + 0) / 1;
}

static void get_pass_heights(uint32_t height, uint32_t heights[7]) {
  heights[0] = (height + 7) / 8;
  heights[1] = (height + 7) / 8;
  heights[2] = (height + 3) / 8;
  heights[3] = (height + 3) / 4;
  heights[4] = (height + 1) / 4;
  heights[5] = (height + 1) / 2;
  heights[6] = (height + 0) / 2;
}

static int allocate_decompression_buffer(PngInfo *pOut, State &state) {
  uint32_t colors = get_colors(state.ColorType);

  size_t stride = 1 + ((size_t)state.Width * state.BitDepth * colors + 7) / 8;
  size_t requiredSpace = state.Height * stride;

  if (state.InterlaceMethod == 1) {
    size_t passWidths[7];
    get_pass_widths(state.Width, passWidths);

    uint32_t passHeights[7];
    get_pass_heights(state.Height, passHeights);

    // get width in bytes
    // no data = no scanline filter byte
    for (int i = 0; i < 7; ++i) {
      if (passWidths[i] != 0)
        passWidths[i] = 1 + (passWidths[i] * state.BitDepth * colors + 7) / 8;
    }

    requiredSpace = 0;
    for (int i = 0; i < 7; ++i) {
      requiredSpace += passWidths[i] * passHeights[i];
    }
  }

  size_t paletteSize = 0;
  if (state.ColorType == 3) {
    paletteSize = (size_t)state.PaletteCount * 3;
    requiredSpace += paletteSize;
  }

  state.Data = (uint8_t *)arena_allocate(state.Arena, requiredSpace);
  state.Size = requiredSpace;

  if (state.ColorType == 3) {
    pOut->Palette = state.Data;
    pOut->PaletteCount = state.PaletteCount;

    state.Data += paletteSize;
    state.Size -= paletteSize;
  }

  state.DeflateState.pOutStream = state.Data;
  state.DeflateState.OutStreamSize = state.Size;

  pOut->Data = state.Data;

  ASSERT_RETURN(state.Data, PNG_OUT_OF_MEMORY);

  return 0;
}

static int chunk_IHDR(const uint8_t *data, size_t len, PngInfo *pOut,
                      State &state) {
  ASSERT_CHUNK_ORDER(STAGE_INITIAL, STAGE_READ_HEADER);
  state.Stage = STAGE_READ_HEADER;

  ASSERT_RETURN(len >= 13, PNG_TOO_SMALL_FOR_HEADER);

  pOut->Width = png_u32(&data[0]);
  pOut->Height = png_u32(&data[4]);
  pOut->BitDepth = png_u8(&data[8]);
  pOut->ColorType = png_u8(&data[9]);
  pOut->InterlaceMethod = png_u8(&data[12]);

  state.Width = png_u32(&data[0]);
  state.Height = png_u32(&data[4]);
  state.BitDepth = png_u8(&data[8]);
  state.ColorType = png_u8(&data[9]);
  state.CompressionMethod = png_u8(&data[10]);
  state.FilterMethod = png_u8(&data[11]);
  state.InterlaceMethod = png_u8(&data[12]);

  console_log_debug("PNG HEADER:");
  console_log_debug("\tWidth: %u", state.Width);
  console_log_debug("\tHeight: %u", state.Height);
  console_log_debug("\tBitDepth: %u", state.BitDepth);
  console_log_debug("\tColorType: %u", state.ColorType);
  console_log_debug("\tCompressionMethod: %u", state.CompressionMethod);
  console_log_debug("\tFilterMethod: %u", state.FilterMethod);
  console_log_debug("\tInterlaceMethod: %u", state.InterlaceMethod);

  ASSERT_RETURN(state.Width != 0 && state.Height != 0,
                PNG_IHDR_INVALID_DIMENSIONS);

  if (state.CompressionMethod != 0)
    return PNG_IHDR_UNSUPPORTED_COMPRESSION_METHOD;

  if (state.FilterMethod != 0)
    return PNG_IHDR_UNSUPPORTED_FILTER_METHOD;

  if (state.InterlaceMethod != 0 && state.InterlaceMethod != 1)
    return PNG_IHDR_UNSUPPORTED_INTERLACE_METHOD;

#define VERIFY_ALLOWED(colorType, ...)                                         \
  case colorType: {                                                            \
    bool valid = false;                                                        \
    constexpr uint8_t allowed[] = {__VA_ARGS__};                               \
    for (int i = 0; i < _countof(allowed); ++i) {                              \
      if (pOut->BitDepth == allowed[i]) {                                      \
        valid = true;                                                          \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (valid)                                                                 \
      break;                                                                   \
                                                                               \
    console_error("Bit depth %u not allowed for colour type %u",               \
                  pOut->BitDepth, pOut->ColorType);                            \
                                                                               \
    return PNG_IHDR_INVALID_BIT_DEPTH;                                         \
  }

  switch (state.ColorType) {
    VERIFY_ALLOWED(0, 1, 2, 4, 8, 16)
    VERIFY_ALLOWED(2, 8, 16)
    VERIFY_ALLOWED(3, 1, 2, 4, 8)
    VERIFY_ALLOWED(4, 8, 16)
    VERIFY_ALLOWED(6, 8, 16)

  default:
    console_error("ColorType %u not allowed", pOut->ColorType);
    return PNG_IHDR_UNSUPPORTED_COLOR_TYPE;
  }

  // done later for palette colours
  if (state.ColorType != 3) {
    PROPAGATE_CODE(allocate_decompression_buffer(pOut, state));
  }

  return 0;
}

static int chunk_PLTE(const uint8_t *data, size_t len, PngInfo *pOut,
                      State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_HEADER, STAGE_READ_PALETTE);
  state.Stage = STAGE_READ_PALETTE;

  ASSERT_RETURN(state.ColorType != 0 && state.ColorType != 4,
                PNG_PLTE_INVALID_COLOR_TYPE);

  ASSERT_RETURN(len % 3 == 0, PNG_PLTE_INVALID_SIZE);

  state.Palette = data;
  state.PaletteCount = (uint32_t)(len / 3);

  PROPAGATE_CODE(allocate_decompression_buffer(pOut, state));

  ASSERT_RETURN(pOut->Palette, PNG_OUT_OF_MEMORY);
  ASSERT_RETURN(state.Palette, PNG_OUT_OF_MEMORY);
  memcpy(pOut->Palette, state.Palette, len);

  return 0;
}

static int chunk_IDAT(const uint8_t *data, size_t len, State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_HEADER, STAGE_END);

  ASSERT_RETURN(state.Data, PNG_OUT_OF_MEMORY);

  ASSERT_RETURN(state.CompressionMethod == 0,
                PNG_IHDR_UNSUPPORTED_COMPRESSION_METHOD);

  CHECK_CODE(
      ResourceLoader_deflate_read_partial(data, len, &state.DeflateState),
      PNG_IDAT_DEFLATE_ERROR);

  state.Stage = STAGE_READ_DATA;

  return 0;
}

static int chunk_IEND(State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_DATA, STAGE_END);
  state.Stage = STAGE_END;

  ASSERT(state.FilterMethod == 0);

  uint32_t colors = get_colors(state.ColorType);

  size_t passWidths[7] = {state.Width};
  uint32_t passHeights[7] = {state.Height};

  int passes = 1;
  if (state.InterlaceMethod == 1) {
    passes = 7;

    get_pass_widths(state.Width, passWidths);
    get_pass_heights(state.Height, passHeights);
  }

  uint8_t *pDataOld = state.Data;
  uint8_t *pDataNew = state.Data;

  for (int pass = 0; pass < passes; ++pass) {
    size_t strideNoFilter =
        (passWidths[pass] * state.BitDepth * colors + 7) / 8;
    uint32_t pixelOffset = colors * ((state.BitDepth + 7) / 8);

    for (size_t scanLine = 0; scanLine < passHeights[pass]; ++scanLine) {
      uint8_t *pLineOld = &pDataOld[(strideNoFilter + 1) * scanLine];
      uint8_t *pLine = &pDataNew[strideNoFilter * scanLine];
      uint8_t filterType = pLineOld[0];

      for (size_t i = 0; i < strideNoFilter; ++i) {
        pLine[i] = pLineOld[i + 1];
      }

      if (filterType == 0) {
      } else if (filterType == 1) {
        for (size_t i = 0; i < strideNoFilter; ++i) {
          uint8_t x = pLine[i];
          uint8_t ra = 0;
          if (i >= pixelOffset) {
            ra = pLine[i - pixelOffset];
          }

          pLine[i] = x + ra;
        }
      } else if (filterType == 2) {
        for (size_t i = 0; i < strideNoFilter; ++i) {
          uint8_t x = pLine[i];
          uint8_t rb = 0;
          if (scanLine >= 1) {
            rb = pLine[i - strideNoFilter];
          }

          pLine[i] = x + rb;
        }
      } else if (filterType == 3) {
        for (size_t i = 0; i < strideNoFilter; ++i) {
          uint8_t x = pLine[i];
          uint8_t ra = 0;
          if (i >= pixelOffset) {
            ra = pLine[i - pixelOffset];
          }
          uint8_t rb = 0;
          if (scanLine >= 1) {
            rb = pLine[i - strideNoFilter];
          }

          pLine[i] = x + ((uint32_t)ra + rb) / 2;
        }
      } else if (filterType == 4) {
        for (size_t i = 0; i < strideNoFilter; ++i) {
          uint8_t x = pLine[i];
          uint8_t ra = 0;
          if (i >= pixelOffset) {
            ra = pLine[i - pixelOffset];
          }
          uint8_t rb = 0;
          if (scanLine >= 1) {
            rb = pLine[i - strideNoFilter];
          }
          uint8_t rc = 0;
          if (i >= pixelOffset && scanLine >= 1) {
            rc = pLine[i - strideNoFilter - pixelOffset];
          }

          int32_t p = ra + rb - rc;
          int32_t pa = abs(p - ra);
          int32_t pb = abs(p - rb);
          int32_t pc = abs(p - rc);

          uint32_t pr;
          if (pa <= pb && pa <= pc)
            pr = ra;
          else if (pb <= pc)
            pr = rb;
          else
            pr = rc;

          pLine[i] = x + (uint8_t)pr;
        }
      }
    }

    pDataOld += (strideNoFilter + 1) * passHeights[pass];
    pDataNew += (strideNoFilter + 0) * passHeights[pass];
  }

  return 0;
}

RESOURCE_LOADER_API code_t
ResourceLoader_decompress_png(const void *pFile, size_t fileSize, PngInfo *pOut,
                              ResourceLoader_arena_t arena) {
  const uint8_t *file = (uint8_t *)pFile;
  const size_t size = fileSize;

  *pOut = PngInfo{};

  ASSERT_RETURN(size >= 8, PNG_TOO_SMALL_FOR_MAGIC_NUMBER);

  uint64_t magicNum = *(uint64_t *)file;
  constexpr uint64_t correctNum = 0x0A1A0A0D474E5089;

  if (magicNum != correctNum)
    return PNG_BAD_MAGIC_NUMBER;

  console_log_debug("Decompressing PNG file...");

  State state{
      .Arena = arena,
  };

  size_t offset = 8;
  while (true) {
    ASSERT_RETURN(size - offset >= 12, PNG_TOO_SMALL_FOR_CHUNK_HEADER);

    const uint8_t *pLength = file + offset;
    const uint8_t *pName = file + offset + 4;
    const uint8_t *pData = file + offset + 8;

    char chunk[5]{};
    memcpy_s(chunk, 4, pName, 4);

    const size_t dataLen = png_u32(pLength);
    const uint32_t chunkId = encode(chunk);

    const bool ancillary = chunk[0] & 0b100000;

    ASSERT_RETURN(size >= offset + 12 + dataLen,
                  PNG_TOO_SMALL_FOR_CHUNK_LENGTH);

    const uint32_t crc = png_u32(pData + dataLen);
    const uint32_t crc1 = calc_crc(pName, dataLen + 4);
    if (crc != crc1)
      return PNG_BAD_CRC;

#define SWITCH_CHUNK(name, ...)                                                \
  {                                                                            \
  case encode(#name):                                                          \
    int result = chunk_##name##(__VA_ARGS__);                                  \
    if (result != 0)                                                           \
      return result;                                                           \
    break;                                                                     \
  }

    switch (chunkId) {
      SWITCH_CHUNK(IHDR, pData, dataLen, pOut, state);
      SWITCH_CHUNK(PLTE, pData, dataLen, pOut, state);
      SWITCH_CHUNK(IDAT, pData, dataLen, state);
      SWITCH_CHUNK(IEND, state);

    default:
      if (ancillary)
        break;

      console_log_debug("Unrecognized required chunk: %s", chunk);

      return PNG_UNSUPPORTED_CHUNK_NAME;
    }

    offset += dataLen + 12;

    if (state.Stage == STAGE_END)
      break;
  }

  ASSERT(size - offset == 0);

  return 0;
}

RESOURCE_LOADER_API code_t
ResourceLoader_deinterlace_png(PngInfo *pPng, ResourceLoader_arena_t arena) {
  ASSERT_RETURN(pPng->InterlaceMethod == 1, PNG_DEINTERLACE_BAD_INPUT);

  uint32_t colors = get_colors(pPng->ColorType);

  size_t scanLineSize = ((size_t)pPng->Width * pPng->BitDepth * colors + 7) / 8;
  size_t requiredSpace = scanLineSize * pPng->Height;

  size_t paletteSize = (size_t)pPng->PaletteCount * 3;
  if (pPng->ColorType == 3) {
    ASSERT_RETURN(pPng->Palette, PNG_DEINTERLACE_BAD_INPUT);
    ASSERT_RETURN(pPng->PaletteCount, PNG_DEINTERLACE_BAD_INPUT);
    requiredSpace += paletteSize;
  }

  uint8_t *pData = (uint8_t *)arena_allocate(arena, requiredSpace);
  size_t dataSize = requiredSpace;

  uint8_t *pPalette = nullptr;

  if (pPng->ColorType == 3) {
    pPalette = pData;

    pData += paletteSize;
    dataSize -= paletteSize;

    memcpy(pPalette, pPng->Palette, paletteSize);
  }

  uint8_t clampedBitDepth = min(pPng->BitDepth, 8);

  uint8_t mask = 0;
  for (uint8_t i = 0; i < clampedBitDepth; ++i) {
    mask = (mask >> 1u) | 0b10000000u;
  }

  size_t widths[7] = {};
  uint32_t heights[7] = {};
  get_pass_widths(pPng->Width, widths);
  get_pass_heights(pPng->Height, heights);

  uint32_t starting_row[7] = {0, 0, 4, 0, 2, 0, 1};
  uint32_t starting_col[7] = {0, 4, 0, 2, 0, 1, 0};
  uint32_t stride_row[7] = {8, 8, 8, 4, 4, 2, 2};
  uint32_t stride_col[7] = {8, 8, 4, 4, 2, 2, 1};

  uint8_t *pInScanLine = pPng->Data;

  memset(pData, 0, scanLineSize * pPng->Height);

  for (uint32_t pass = 0; pass < 7; ++pass) {

    uint32_t clampedWidth =
        ((uint32_t)widths[pass] * pPng->BitDepth) / clampedBitDepth;
    uint32_t height = heights[pass];

    for (uint32_t row = 0; row < height; ++row) {

      uint8_t *pOutLine =
          &pData[((size_t)row * stride_row[pass] + starting_row[pass]) *
                 scanLineSize];

      for (uint32_t col = 0; col < (uint32_t)clampedWidth; ++col) {

        uint32_t out_x = starting_col[pass] + (col * stride_col[pass]);

        uint32_t in_byte_x = (col * clampedBitDepth) / 8;
        uint32_t in_bit = (col * clampedBitDepth) % 8;

        uint8_t pixel = pInScanLine[in_byte_x] & (mask >> in_bit);

        pOutLine[out_x] = (pOutLine[out_x] << 1) | pixel;
      }

      pInScanLine += ((uint32_t)widths[pass] * pPng->BitDepth * colors + 7) / 8;
    }
  }

  pPng->InterlaceMethod = 0;
  pPng->Palette = pPalette;
  pPng->Data = pData;

  return 0;
}

RESOURCE_LOADER_API code_t
ResourceLoader_png_to_rgba8(PngInfo *pPng, ResourceLoader_arena_t arena) {
  ASSERT_RETURN(pPng->InterlaceMethod == 0, PNG_CONVERT_UNSUPPORTED_INTERLACE);

  size_t requiredSpace = (size_t)pPng->Width * pPng->Height * 4;

  uint8_t *pData = (uint8_t *)arena_allocate(arena, requiredSpace);

  if (pPng->ColorType == 6) {
    size_t skip = pPng->BitDepth / 8;
    for (size_t i = 0; i < requiredSpace; ++i) {
      // NOTE: This is slightly innacurate. Refer to PNG spec for a more
      // accurate method.
      pData[i] = pPng->Data[i * skip];
    }
  } else if (pPng->ColorType == 2) {
    size_t skip = pPng->BitDepth / 8;
    for (size_t i = 0; i < (size_t)pPng->Width * pPng->Height; ++i) {
      // NOTE: This is slightly innacurate. Refer to PNG spec for a more
      // accurate method.
      size_t inX = i * 3;
      size_t outX = i * 4;
      pData[outX + 0] = pPng->Data[(inX + 0) * skip];
      pData[outX + 1] = pPng->Data[(inX + 1) * skip];
      pData[outX + 2] = pPng->Data[(inX + 2) * skip];
      pData[outX + 3] = 255;
    }
  } else if (pPng->ColorType == 4) {
    size_t skip = pPng->BitDepth / 8;
    for (size_t i = 0; i < (size_t)pPng->Width * pPng->Height; ++i) {
      // NOTE: This is slightly innacurate. Refer to PNG spec for a more
      // accurate method.
      size_t inX = i * 2;
      size_t outX = i * 4;
      pData[outX + 0] = pPng->Data[(inX + 0) * skip];
      pData[outX + 1] = pPng->Data[(inX + 0) * skip];
      pData[outX + 2] = pPng->Data[(inX + 0) * skip];
      pData[outX + 3] = pPng->Data[(inX + 1) * skip];
    }
  } else if (pPng->ColorType == 3) {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < pPng->BitDepth; ++i) {
      mask = (mask >> 1u) | 0b10000000u;
    }
    for (size_t i = 0; i < (size_t)pPng->Width * pPng->Height; ++i) {

      size_t outX = i * 4;
      size_t inByte = (i * pPng->BitDepth) / 8;
      uint8_t inBit = (i * pPng->BitDepth) % 8;

      uint8_t index = (pPng->Data[inByte] & (mask >> inBit)) >>
                      (8u - pPng->BitDepth - inBit);

      ASSERT(index < pPng->PaletteCount);

      uint8_t r = pPng->Palette[index * 3 + 0];
      uint8_t g = pPng->Palette[index * 3 + 1];
      uint8_t b = pPng->Palette[index * 3 + 2];

      pData[outX + 0] = r;
      pData[outX + 1] = g;
      pData[outX + 2] = b;
      pData[outX + 3] = 255;
    }
  } else if (pPng->ColorType == 0 && pPng->BitDepth <= 8) {
    uint8_t mask = 0;
    uint8_t max = 0;
    for (uint8_t i = 0; i < pPng->BitDepth; ++i) {
      mask = (mask >> 1u) | 0b10000000u;
      max = (max << 1u) | 0b1u;
    }
    for (size_t i = 0; i < (size_t)pPng->Width * pPng->Height; ++i) {

      size_t outX = i * 4;
      size_t inByte = (i * pPng->BitDepth) / 8;
      uint8_t inBit = (i * pPng->BitDepth) % 8;

      uint8_t value = (pPng->Data[inByte] & (mask >> inBit)) >>
                      (8u - pPng->BitDepth - inBit);
      value = (uint8_t)(((float)value / max) * 255);

      pData[outX + 0] = value;
      pData[outX + 1] = value;
      pData[outX + 2] = value;
      pData[outX + 3] = 255;
    }
  } else if (pPng->ColorType == 0 && pPng->BitDepth == 16) {
    for (size_t i = 0; i < (size_t)pPng->Width * pPng->Height; ++i) {
      // NOTE: This is slightly innacurate. Refer to PNG spec for a more
      // accurate method.
      size_t outX = i * 4;

      uint8_t value = pPng->Data[i * 2];

      pData[outX + 0] = value;
      pData[outX + 1] = value;
      pData[outX + 2] = value;
      pData[outX + 3] = 255;
    }
  } else {
    ASSERT_RETURN(false, PNG_CONVERT_UNSUPPORTED_FORMAT);
  }

  pPng->ColorType = 6;
  pPng->BitDepth = 8;
  pPng->Data = pData;
  return 0;
}
