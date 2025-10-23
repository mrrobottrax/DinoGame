#include "pch.h"

#include "arenas_private.h"
#include "file.h"

#define LF_FAILED_CONVERT 1;
#define LF_FAILED_OPEN 2;
#define LF_FAILED_SIZE 3;
#define LF_OUT_OF_MEMORY 4;
#define LF_FAILED_READ 5;

RESOURCE_LOADER_API code_t
ResourceLoader_load_file(const char *path, void **ppFile, size_t *pFileSize,
                         ResourceLoader_arena_t arena) {
  *ppFile = nullptr;
  *pFileSize = 0;

  arena_reset(arena);

  constexpr wchar_t prefix[] = L"content\\";
  constexpr size_t prefixLen = sizeof(prefix) / sizeof(wchar_t) - 1;

  int wcLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
  if (wcLen == 0)
    return LF_FAILED_CONVERT;

  const size_t required = (prefixLen + wcLen) * sizeof(wchar_t);
  void *const buffer = arena_allocate(arena, required);

  memcpy_s(buffer, required, prefix, prefixLen * sizeof(wchar_t));

  void *const pathStart = (wchar_t *)buffer + prefixLen;
  const size_t pathSize = required / sizeof(wchar_t) - prefixLen;

  wcLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, (LPWSTR)pathStart,
                              (int)pathSize);
  if (wcLen == 0)
    return LF_FAILED_CONVERT;

  HANDLE hFile = CreateFile(
      (LPWSTR)buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if (hFile == INVALID_HANDLE_VALUE)
    return LF_FAILED_OPEN;

  LARGE_INTEGER fileSize;
  ASSERT(GetFileSizeEx(hFile, &fileSize));

#ifdef DEBUG
  console_log("Reading file %s, Size: %llu", path, fileSize.QuadPart);
#endif // DEBUG
  ASSERT_ALWAYS(fileSize.QuadPart < MAXDWORD32);

  arena_reset(arena);

  void *alloc = arena_allocate(arena, fileSize.QuadPart);
  if (!alloc)
    return LF_OUT_OF_MEMORY;

  *ppFile = alloc;
  *pFileSize = fileSize.QuadPart;

  ASSERT_WIN_EXP(ReadFile(hFile, alloc, (DWORD)fileSize.QuadPart, NULL, NULL));

  CloseHandle(hFile);

  return 0;
}