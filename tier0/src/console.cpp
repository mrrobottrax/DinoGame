#include "pch.h"

#include "error.h"

#include "console.h"

static HANDLE s_hLogFile;
static CRITICAL_SECTION s_LogLock;

constexpr size_t k_LogFilePrintfBufferLength = 1 << 12;
static char s_LogFilePrintfBuffer[k_LogFilePrintfBufferLength];

void console_create() {
#ifdef T0_CONSOLE
  // create console
  if (!AllocConsole()) {
    CRASH_WIN("AllocConsole failed");
  }

  FILE *stream;
  freopen_s(&stream, "CONOUT$", "w", stdout);
  freopen_s(&stream, "CONOUT$", "r", stdin);
  freopen_s(&stream, "CONOUT$", "w", stderr);

  SetConsoleOutputCP(CP_UTF8);
#endif

  // create file
  s_hLogFile = CreateFile(
      L"log.txt", GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
  if (s_hLogFile == INVALID_HANDLE_VALUE) {
    CRASH_WIN("Failed to create log file");
  }

  InitializeCriticalSection(&s_LogLock);
}

void console_free() {
  EnterCriticalSection(&s_LogLock);
  FlushFileBuffers(s_hLogFile);
  CloseHandle(s_hLogFile);
  LeaveCriticalSection(&s_LogLock);

#ifdef T0_CONSOLE
  FreeConsole();
#endif
}

void console_print_va(const char format[], va_list args) {
  va_list args2;
  va_copy(args2, args);

  vprintf(format, args);

  if (s_hLogFile != NULL && s_LogFilePrintfBuffer) {
    EnterCriticalSection(&s_LogLock);
    vsnprintf_s(s_LogFilePrintfBuffer, k_LogFilePrintfBufferLength, _TRUNCATE,
                format, args2);

    size_t len =
        strnlen_s(s_LogFilePrintfBuffer, k_LogFilePrintfBufferLength - 1);

    if (!WriteFile(s_hLogFile, s_LogFilePrintfBuffer, (DWORD)len, NULL, NULL)) {
      printf("[CRASH] Failed to write to log!");
      LeaveCriticalSection(&s_LogLock);
      CRASH_IMMEDIATE();
      return;
    }
    LeaveCriticalSection(&s_LogLock);
  }

  va_end(args2);
}

void console_print(const char format[], ...) {
  va_list args;
  va_start(args, format);
  console_print_va(format, args);
  va_end(args);
}

void console_println(const char format[], ...) {
  va_list args;
  va_start(args, format);
  console_print_va(format, args);
  va_end(args);
  console_print("\r\n");
}

void console_println_va(const char format[], va_list args) {
  console_print_va(format, args);
  console_print("\r\n");
}

void console_log(const char format[], ...) {
  EnterCriticalSection(&s_LogLock);
  console_print("[LOG] ");

  va_list args;
  va_start(args, format);
  console_print_va(format, args);
  va_end(args);

  console_print("\r\n");
  LeaveCriticalSection(&s_LogLock);
}

void console_warn(const char format[], ...) {
  EnterCriticalSection(&s_LogLock);
  console_print("[WARNING] ");

  va_list args;
  va_start(args, format);
  console_print_va(format, args);
  va_end(args);

  console_print("\r\n");
  LeaveCriticalSection(&s_LogLock);
}

void console_error(const char format[], ...) {
  EnterCriticalSection(&s_LogLock);
  console_print("[ERROR] ");

  va_list args;
  va_start(args, format);
  console_print_va(format, args);
  va_end(args);

  console_print("\r\n");
  LeaveCriticalSection(&s_LogLock);
}