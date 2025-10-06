#include "pch.h"

#include "buffer.h"
#include "file.h"

#define LF_FAILED_CONVERT 1;
#define LF_FAILED_OPEN 2;
#define LF_FAILED_SIZE 3;
#define LF_FAILED_ALLOC 4;
#define LF_FAILED_READ 5;

/// <summary>
/// Load file into file buffer.
/// </summary>
IMAGE_LOADER_API int ResourceLoader_load_file(const char *path) {
  constexpr wchar_t prefix[] = L"content\\";
  constexpr size_t prefixLen = sizeof(prefix) / sizeof(wchar_t) - 1;
  memcpy_s(g_Buffer, g_BufferSize, prefix, prefixLen * sizeof(wchar_t));

  void *bufferStart = (wchar_t *)g_Buffer + prefixLen;
  size_t bufferSize = g_BufferSize / sizeof(wchar_t) - prefixLen;

  int wcLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, (LPWSTR)bufferStart,
                                  (int)bufferSize);
  if (wcLen == 0)
    return LF_FAILED_CONVERT;

  HANDLE hFile = CreateFile(
      (LPWSTR)g_Buffer, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if (hFile == NULL)
    return LF_FAILED_OPEN;

  LARGE_INTEGER fileSize;
  if (!GetFileSizeEx(hFile, &fileSize))
    return LF_FAILED_SIZE;

#ifdef DEBUG
  console_log("Reading file %s\r\nSize: %llu", path, fileSize.QuadPart);
  ASSERT_ALWAYS(fileSize.QuadPart < MAXDWORD32);
#endif // DEBUG

  arena0_reset();
  void *alloc = arena0_allocate(fileSize.QuadPart);
  if (!alloc)
    return LF_FAILED_ALLOC;

  if (!ReadFile(hFile, g_Buffer, (DWORD)fileSize.QuadPart, NULL, NULL))
    return LF_FAILED_READ;

  CloseHandle(hFile);

  return 0;
}