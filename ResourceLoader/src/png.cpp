#include "pch.h"

#include "arenas_private.h"
#include "deflate.h"
#include "png.h"
#include "zlib.h"

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

#define PNG_IDAT_ZLIB_HEADER_ERROR MAKE_ERROR(05, 00, 00)
#define PNG_IDAT_ZLIB_ADLER_ERROR MAKE_ERROR(05, 00, 01)
#define PNG_IDAT_ZLIB_UNSUPPORTED_CM MAKE_ERROR(05, 00, 02)

#define PNG_IDAT_DEFLATE_HEADER_ERROR MAKE_ERROR(05, 01, 00)

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

static int chunk_IHDR(const uint8_t *data, size_t len, PngInfo *pOut,
                      State &state) {
  ASSERT_CHUNK_ORDER(STAGE_INITIAL, STAGE_READ_HEADER);
  state.Stage = STAGE_READ_HEADER;

  ASSERT_RETURN(len >= 13, PNG_TOO_SMALL_FOR_HEADER);

  pOut->Width = png_u32(&data[0]);
  pOut->Height = png_u32(&data[4]);
  pOut->BitDepth = png_u8(&data[8]);
  pOut->ColorType = png_u8(&data[9]);

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

  size_t colors = 0;
  switch (state.ColorType) {
  case 0:
    colors = 1;
    break;
  case 2:
    colors = 3;
    break;
  case 3:
    colors = 3;
    break;
  case 4:
    colors = 2;
    break;
  case 6:
    colors = 4;
    break;
  }
  ASSERT(colors != 0);
  size_t requiredSpace = (size_t)state.Width * state.Height *
                         (((size_t)state.BitDepth + 7) / 8) * colors;

  constexpr size_t k_ScanlinePrefix = 1;
  requiredSpace += state.Width * k_ScanlinePrefix;

  state.Data = (uint8_t *)arena_allocate(state.Arena, requiredSpace);
  state.Size = requiredSpace;

  state.DeflateState.pOutStream = state.Data;
  state.DeflateState.OutStreamSize = state.Size;

  pOut->Data = state.Data;
  pOut->Size = state.Size;

  return 0;
}

static int chunk_PLTE(const uint8_t *data, size_t len, State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_HEADER, STAGE_READ_PALETTE);
  state.Stage = STAGE_READ_PALETTE;

  ASSERT_RETURN(state.ColorType != 0 && state.ColorType != 4,
                PNG_PLTE_INVALID_COLOR_TYPE);

  ASSERT_RETURN(len % 3 == 0, PNG_PLTE_INVALID_SIZE);

  state.Palette = data;

  return 0;
}

static int chunk_IDAT(const uint8_t *data, size_t len, State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_HEADER, STAGE_END);

  ASSERT_RETURN(state.CompressionMethod == 0,
                PNG_IHDR_UNSUPPORTED_COMPRESSION_METHOD);

  // Read ZLIB header
  if (state.Stage < STAGE_READ_DATA) {
    ResourceLoader_Zlib_Header header;
    CHECK_CODE(ResourceLoader_zlib_read_header(data, len, &header),
               PNG_IDAT_ZLIB_HEADER_ERROR);

    ASSERT_RETURN(header.CM == 8, PNG_IDAT_ZLIB_UNSUPPORTED_CM);
    ASSERT_RETURN(header.CINFO <= 7, PNG_IDAT_ZLIB_UNSUPPORTED_CM);

    data += header.HeaderSize;
    len -= header.HeaderSize;
  }

  CHECK_CODE(
      ResourceLoader_deflate_read_partial(data, len, &state.DeflateState),
      PNG_IDAT_DEFLATE_HEADER_ERROR);

  state.Stage = STAGE_READ_DATA;

  return 0;
}

static int chunk_IEND(const uint8_t *data, size_t len, State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_DATA, STAGE_END);
  state.Stage = STAGE_END;

  return 0;
}

static int chunk_sRGB(const uint8_t *data, size_t len, PngInfo *pOut,
                      State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_HEADER, STAGE_READ_PALETTE);

  return 1;
}

RESOURCE_LOADER_API int
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

    console_log_debug("Reading chunk: %s", chunk);

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
      SWITCH_CHUNK(PLTE, pData, dataLen, state);
      SWITCH_CHUNK(IDAT, pData, dataLen, state);
      SWITCH_CHUNK(IEND, pData, dataLen, state);
      SWITCH_CHUNK(sRGB, pData, dataLen, pOut, state);

    default:
      if (ancillary)
        break;

      console_log_debug("Unrecognized required chunk: %s", chunk);

      return PNG_UNSUPPORTED_CHUNK_NAME;
    }

    offset += dataLen + 12;

    if (state.Stage == STAGE_END)
      break;

    ASSERT_RETURN(state.Data, PNG_OUT_OF_MEMORY);
  }

  ASSERT(size - offset == 0);

  return 0;
}
