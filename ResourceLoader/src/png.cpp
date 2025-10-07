#include "pch.h"

#include "arenas_private.h"
#include "png.h"

#define PNG_TOO_SMALL 1;
#define PNG_BAD_MAGIC 2;
#define PNG_INCOMPLETE 3;
#define PNG_BAD_CRC 4;
#define PNG_UNRECOGNIZED_CHUNK 5;
#define PNG_BAD_DIMENSIONS 6;
#define PNG_UNSUPPORTED_FORMAT 7;
#define PNG_INVALID_FORMAT 8;
#define PNG_BAD_CHUNK_ORDERING 9;
#define PNG_OUT_OF_MEMORY 10;

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
  uint8_t *Data;
  size_t Size;
  const uint8_t *Palette;
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
  uint32_t o = 0;

  char *p = (char *)pValue;
  char *p1 = (char *)&o;

  p1[0] = p[3];
  p1[1] = p[2];
  p1[2] = p[1];
  p1[3] = p[0];

  return o;
}

static uint8_t png_u8(const uint8_t *pValue) { return *pValue; }

#define ASSERT_CHUNK_ORDER(afterOrDuring, before)                              \
  if (state.Stage < afterOrDuring || state.Stage >= before)                    \
    return PNG_BAD_CHUNK_ORDERING;

static int chunk_IHDR(const uint8_t *data, size_t len, PngInfo *pOut,
                      State &state) {
  ASSERT_CHUNK_ORDER(STAGE_INITIAL, STAGE_READ_HEADER);
  state.Stage = STAGE_READ_HEADER;

  if (len < 13)
    return PNG_INCOMPLETE;

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

  if (state.Width == 0 || state.Height == 0)
    return PNG_BAD_DIMENSIONS;

  if (state.CompressionMethod != 0)
    return PNG_UNSUPPORTED_FORMAT;

  if (state.FilterMethod != 0)
    return PNG_UNSUPPORTED_FORMAT;

  if (state.InterlaceMethod != 0 && state.InterlaceMethod != 1)
    return PNG_UNSUPPORTED_FORMAT;

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
    return PNG_INVALID_FORMAT;                                                 \
  }

  switch (state.ColorType) {
    VERIFY_ALLOWED(0, 1, 2, 4, 8, 16)
    VERIFY_ALLOWED(2, 8, 16)
    VERIFY_ALLOWED(3, 1, 2, 4, 8)
    VERIFY_ALLOWED(4, 8, 16)
    VERIFY_ALLOWED(6, 8, 16)

  default:
    console_error("Colour type %u not allowed", pOut->ColorType);
    return PNG_UNSUPPORTED_FORMAT;
  }

  size_t colors;
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
  size_t requiredSpace = (size_t)state.Width * state.Height *
                         (((size_t)state.BitDepth + 7) / 8) * colors;
  state.Data = (uint8_t *)arena_allocate(state.Arena, requiredSpace);
  state.Size = requiredSpace;

  pOut->Data = state.Data;
  pOut->Size = state.Size;

  return 0;
}

static int chunk_PLTE(const uint8_t *data, size_t len, State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_HEADER, STAGE_READ_PALETTE);
  state.Stage = STAGE_READ_PALETTE;

  if (state.ColorType == 0 || state.ColorType == 4)
    return PNG_INVALID_FORMAT;

  if (len % 3 != 0)
    return PNG_INVALID_FORMAT;

  state.Palette = data;

  return 0;
}

static int chunk_IDAT(const uint8_t *data, size_t len, State &state) {
  ASSERT_CHUNK_ORDER(STAGE_READ_HEADER, STAGE_READ_DATA);
  state.Stage = STAGE_READ_DATA;

  return 1;
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
  PngInfo &out = *pOut;

  if (size < 8)
    return PNG_TOO_SMALL;

  uint64_t magicNum = *(uint64_t *)file;
  constexpr uint64_t correctNum = 0x0A1A0A0D474E5089;

  if (magicNum != correctNum)
    return PNG_BAD_MAGIC;

  console_log_debug("Decompressing PNG file...");

  State state{
      .Arena = arena,
  };

  size_t offset = 8;
  while (true) {
    if (size - offset < 12)
      return PNG_INCOMPLETE;

    const uint8_t *pLength = file + offset;
    const uint8_t *pName = file + offset + 4;
    const uint8_t *pData = file + offset + 8;

    char chunk[5]{};
    memcpy_s(chunk, 4, pName, 4);

    const size_t dataLen = png_u32(pLength);
    const uint32_t chunkId = encode(chunk);

    const bool ancillary = chunk[0] & 0b100000;

    if (size < offset + 12 + dataLen)
      return PNG_INCOMPLETE;

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

      return PNG_UNRECOGNIZED_CHUNK;
    }

    offset += dataLen + 12;

    if (state.Stage == STAGE_END)
      break;

    if (!state.Data)
      return PNG_OUT_OF_MEMORY;
  }

  ASSERT(size - offset == 0);

  return 0;
}
