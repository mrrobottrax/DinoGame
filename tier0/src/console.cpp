#include "pch.h"

#include "error.h"

#include "console.h"

static HANDLE s_hLogFile;
static CRITICAL_SECTION s_LogLock;

static char *s_LogFilePrintfBuffer;
static size_t s_LogFilePrintfBufferLen = 1024;

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

  s_LogFilePrintfBuffer = (char *)malloc(s_LogFilePrintfBufferLen);
  if (s_LogFilePrintfBuffer == nullptr) {
    CRASH("Failed to allocate file buffer");
  }
}

void console_free_filebuffer() {
  free(s_LogFilePrintfBuffer);
  s_LogFilePrintfBuffer = nullptr;
}

void console_free() {
  EnterCriticalSection(&s_LogLock);
  FlushFileBuffers(s_hLogFile);
  CloseHandle(s_hLogFile);
  console_free_filebuffer();
  LeaveCriticalSection(&s_LogLock);

#ifdef T0_CONSOLE
  FreeConsole();
#endif
}

void console_print_va(const char format[], va_list args) {
  va_list args2, args3;
  va_copy(args2, args);
  va_copy(args3, args);

  vprintf(format, args);

  if (s_hLogFile != NULL && s_LogFilePrintfBuffer) {
    size_t lenRequired = _vscprintf(format, args2);

    EnterCriticalSection(&s_LogLock);
    if (s_LogFilePrintfBufferLen <= lenRequired + 1) {
      while (s_LogFilePrintfBufferLen <= lenRequired + 1) {
        s_LogFilePrintfBufferLen *= 2;
      }

      free(s_LogFilePrintfBuffer);
      s_LogFilePrintfBuffer = (char *)malloc(s_LogFilePrintfBufferLen);

      if (s_LogFilePrintfBuffer == nullptr) {
        printf("[CRASH] Failed to allocate memory!");
        LeaveCriticalSection(&s_LogLock);
        CRASH_IMMEDIATE();
        return;
      }
    }

    lenRequired = vsnprintf_s(s_LogFilePrintfBuffer, s_LogFilePrintfBufferLen,
                              s_LogFilePrintfBufferLen - 1, format, args3);

    if (lenRequired > MAXDWORD) {
      console_log("String too long");
      lenRequired = MAXDWORD;
    }

    if (lenRequired > s_LogFilePrintfBufferLen) {
      console_log("String too long");
      lenRequired = s_LogFilePrintfBufferLen;
    }

    if (!WriteFile(s_hLogFile, s_LogFilePrintfBuffer, (DWORD)lenRequired, NULL,
                   NULL)) {
      printf("[CRASH] Failed to write to log!");
      LeaveCriticalSection(&s_LogLock);
      CRASH_IMMEDIATE();
      return;
    }
    LeaveCriticalSection(&s_LogLock);
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