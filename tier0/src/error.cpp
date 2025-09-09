#include "pch.h"

#include "console.h"
#include "error.h"

__declspec(thread) static char *s_lastError = nullptr;

static void set_error_va(const char *format, va_list args) {
  va_list args_required;
  va_copy(args_required, args);
  size_t required = _vscprintf(format, args_required);
  va_end(args_required);

  s_lastError = (char *)malloc(required + 1);
  if (s_lastError) {
    va_list args_log;
    va_copy(args_log, args);
    console_log_error_va(format, args_log);
    va_end(args_log);

    va_list args_vsnprintf;
    va_copy(args_vsnprintf, args);
    if (vsnprintf_s(s_lastError, required + 1, _TRUNCATE, format,
                    args_vsnprintf) < 0) {
      console_log_error("Formatting failure");
    }
    va_end(args_vsnprintf);
  }
}

void set_error(const char *format, ...) {
  free_error();

  va_list args;
  va_start(args, format);

  set_error_va(format, args);

  va_end(args);
}

void set_windows_error(const char *format, ...) {
  DWORD err = GetLastError();

  LPWSTR wideErr = nullptr;
  DWORD wideErrLen = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                                        FORMAT_MESSAGE_IGNORE_INSERTS |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    NULL, err, 0, (LPWSTR)&wideErr, 0, NULL);

  char *mbErr = nullptr;
  size_t mbErrLen = 0;
  if (wideErr != nullptr) {
    mbErrLen = WideCharToMultiByte(CP_UTF8, 0, wideErr, wideErrLen + 1,
                                   NULL, 0, NULL, NULL);

    mbErr = (char *)malloc(mbErrLen);
    WideCharToMultiByte(CP_UTF8, 0, wideErr, wideErrLen + 1, mbErr,
                        (int)mbErrLen, NULL, NULL);

    mbErrLen -= 1;

    LocalFree(wideErr);
  }

  constexpr char affix[] = ":\r\n";
  constexpr size_t affixLen = sizeof(affix) - 1;

  size_t len = strnlen_s(format, 4096);
  char *finalFormat = (char *)malloc(len + affixLen + mbErrLen + 1);
  strncpy_s(finalFormat, len + affixLen + mbErrLen + 1, format, len);
  strncpy_s(finalFormat + len, affixLen + mbErrLen + 1, affix, affixLen);
  strncpy_s(finalFormat + len + affixLen, mbErrLen + 1, mbErr, mbErrLen);

  va_list args;
  va_start(args, format);

  set_error_va(finalFormat, args);

  va_end(args);
}

const char *get_error() { return s_lastError; }

void free_error() {
  free(s_lastError);
  s_lastError = nullptr;
}
