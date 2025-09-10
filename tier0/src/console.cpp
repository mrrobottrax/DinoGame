#include "pch.h"

#include "error.h"

#include "console.h"

static HANDLE s_hLogFile;
static CRITICAL_SECTION s_logLock;

static char *s_fileBuffer;
static size_t s_fileBufferLen = 1024;

void console_create() {
  // create console
  if (!AllocConsole()) {
    CRASH_WIN("AllocConsole failed");
  }

  FILE *stream;
  freopen_s(&stream, "CONOUT$", "w", stdout);
  freopen_s(&stream, "CONOUT$", "r", stdin);
  freopen_s(&stream, "CONOUT$", "w", stderr);

  SetConsoleOutputCP(CP_UTF8);

  // create file
  s_hLogFile = CreateFile(
      L"log.txt", GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
  if (s_hLogFile == INVALID_HANDLE_VALUE) {
    CRASH_WIN("Failed to create log file");
  }

  InitializeCriticalSection(&s_logLock);

  s_fileBuffer = (char *)malloc(s_fileBufferLen);
  if (s_fileBuffer == nullptr) {
    CRASH("Failed to allocate file buffer");
  }
}

void console_free_filebuffer() {
  free(s_fileBuffer);
  s_fileBuffer = nullptr;
}

void console_free() {
  EnterCriticalSection(&s_logLock);
  CloseHandle(s_hLogFile);
  LeaveCriticalSection(&s_logLock);

  console_free_filebuffer();

  FreeConsole();
}

void console_line() { console_print("\n"); }

void console_print_va(const char format[], va_list args) {
  va_list args2, args3;
  va_copy(args2, args);
  va_copy(args3, args);

  vprintf(format, args);

  if (s_hLogFile != NULL && s_fileBuffer) {
    size_t lenRequired = _vscprintf(format, args2);

    EnterCriticalSection(&s_logLock);
    if (s_fileBufferLen <= lenRequired + 1) {
      while (s_fileBufferLen <= lenRequired + 1) {
        s_fileBufferLen *= 2;
      }

      free(s_fileBuffer);
      s_fileBuffer = (char *)malloc(s_fileBufferLen);

      if (s_fileBuffer == nullptr) {
        printf("[CRASH] Failed to allocate memory");
        LeaveCriticalSection(&s_logLock);
        exit(T0_OUT_OF_MEMORY);
      }
    }

    lenRequired = vsnprintf_s(s_fileBuffer, s_fileBufferLen,
                              s_fileBufferLen - 1, format, args3);

    if (lenRequired > MAXDWORD) {
      console_log("String too long");
      lenRequired = MAXDWORD;
    }

    if (!WriteFile(s_hLogFile, s_fileBuffer, (DWORD)lenRequired, NULL, NULL)) {
      printf("[CRASH] Failed to write to log!!");
      LeaveCriticalSection(&s_logLock);
      exit(T0_OUT_OF_MEMORY);
    }
    LeaveCriticalSection(&s_logLock);
  }

  va_end(args2);
  va_end(args3);
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
  console_print("\n");
}

void console_println_va(const char format[], va_list args) {
  console_print_va(format, args);
  console_print("\n");
}

void console_log(const char format[], ...) {
  console_print("[LOG] ");

  va_list args;
  va_start(args, format);
  console_print_va(format, args);
  va_end(args);

  console_print("\n");
}

void console_log_warn(const char format[], ...) {
  console_print("[WARNING] ");

  va_list args;
  va_start(args, format);
  console_print_va(format, args);
  va_end(args);

  console_print("\n");
}

void console_log_error(const char format[], ...) {
  console_print("[ERROR] ");

  va_list args;
  va_start(args, format);
  console_print_va(format, args);
  va_end(args);

  console_print("\n");
}