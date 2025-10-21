#include "pch.h"

#include "console.h"
#include "console_private.h"
#include "error.h"
#include "error_private.h"

static CRITICAL_SECTION s_CrashLock;

constexpr size_t k_ErrorBufferLength = 1 << 12;
static char s_MbErrorBuffer[k_ErrorBufferLength];
static char s_MbFormatBuffer[k_ErrorBufferLength];
static wchar_t s_WcErrorBuffer[k_ErrorBufferLength];

static void write_minidump() {
  console_log("Writing minidump...");
  HANDLE hFile = CreateFile(L"crash.dmp", GENERIC_WRITE, 0, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    console_error("Failed to write minidump");
    return;
  }

  if (!MiniDumpWriteDump(
          GetCurrentProcess(), GetCurrentProcessId(), hFile,
          (MINIDUMP_TYPE)(MiniDumpWithDataSegs |
                          MiniDumpWithIndirectlyReferencedMemory |
                          MiniDumpScanMemory | MiniDumpWithFullMemory |
                          MiniDumpWithHandleData | MiniDumpWithThreadInfo),
          nullptr, nullptr, nullptr)) {
    console_error("Failed to write minidump 2");
  }

  CloseHandle(hFile);
}

static void print_stack() {
  constexpr size_t kMaxDepth = 100;
  console_println("\n!! STACK TRACE !!\nNote: Only includes up to depth %llu",
                  kMaxDepth);

  console_println("Thread ID: %u", GetCurrentThreadId());

  PWSTR threadDesc = nullptr;
  if (SUCCEEDED(GetThreadDescription(GetCurrentThread(), &threadDesc)) &&
      threadDesc && threadDesc[0] != L'\0') {
    console_println("Thread Description: %ls", threadDesc);
  }

  void *stack[kMaxDepth];
  USHORT frames = CaptureStackBackTrace(0, kMaxDepth, stack, NULL);

  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);
  SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

  constexpr size_t k_NameLen = 512;
  char buffer[sizeof(SYMBOL_INFO) + k_NameLen];
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)&buffer;
  symbol->MaxNameLen = k_NameLen;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  IMAGEHLP_LINE64 line = {};
  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
  DWORD displacement = 0;

  for (USHORT i = 0; i < frames; ++i) {
    DWORD64 address = (DWORD64)(stack[i]);

    if (SymFromAddr(process, address, 0, symbol)) {
      if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
        console_println("%s (%s:%lu)", symbol->Name, line.FileName,
                        line.LineNumber);
      } else {
        console_println("%s (no line info)", symbol->Name);
      }
    } else {
      console_println("Unknown symbol at address 0x%llx", address);
    }
  }

  console_line();
}

/// <summary>
/// Uses error stored in s_MbErrorBuffer
/// </summary>
static void error_popup() {
  int wcLen = MultiByteToWideChar(CP_UTF8, 0, s_MbErrorBuffer, -1, NULL, 0);
  if (wcLen > k_ErrorBufferLength) {
    console_error("Error popup truncation");
  }
  MultiByteToWideChar(CP_UTF8, 0, s_MbErrorBuffer, -1, s_WcErrorBuffer,
                      k_ErrorBufferLength);
  MessageBoxExW(NULL, s_WcErrorBuffer, L"Error", MB_OK | MB_ICONERROR, 0);
}

/// <summary>
/// Print error and copy into s_MbErrorBuffer.
/// </summary>
static void set_error_va(const char *format, va_list args) {
  console_line();
  console_println("!! CRASH !!");

  if (!format)
    return;

  va_list args_log;
  va_copy(args_log, args);
  console_println_va(format, args_log);
  va_end(args_log);

  va_list args_vsnprintf;
  va_copy(args_vsnprintf, args);
  vsnprintf_s(s_MbErrorBuffer, k_ErrorBufferLength, _TRUNCATE, format,
              args_vsnprintf);
  va_end(args_vsnprintf);
}

static void set_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  set_error_va(format, args);
  va_end(args);
}

/// <summary>
/// Copy format string + windows message into s_MbFormatBuffer.
/// </summary>
static void append_windows_message(const char *format, DWORD error) {
  if (!FormatMessageW(
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
          error, 0, s_WcErrorBuffer, k_ErrorBufferLength, NULL)) {
    _snwprintf_s(s_WcErrorBuffer, k_ErrorBufferLength, _TRUNCATE,
                 L"Truncation. Win message number: %u", error);
  }

  constexpr char affix[] = "\r\nWin: ";
  constexpr size_t affixLen = sizeof(affix) - 1;

  // no null char
  size_t len = strnlen_s(format, k_ErrorBufferLength - affixLen - 1);

  strncpy_s(s_MbFormatBuffer, k_ErrorBufferLength, format, len);
  strncat_s(s_MbFormatBuffer, k_ErrorBufferLength, affix, affixLen);
  WideCharToMultiByte(
      CP_UTF8, 0, s_WcErrorBuffer, -1, s_MbFormatBuffer + len + affixLen,
      (int)(k_ErrorBufferLength - len - affixLen - 1), NULL, NULL);
}

/// <summary>
/// Copy format string + return code into s_MbFormatBuffer. Corrupts
/// s_MbErrorBuffer.
/// </summary>
static void append_return_code(const char *format, int code) {
  constexpr char codeFormat[] = "\r\nReturn code: %i";

  _snprintf_s(s_MbErrorBuffer, k_ErrorBufferLength, _TRUNCATE, codeFormat,
              code);

  // no null char
  size_t len = strnlen_s(format, k_ErrorBufferLength - 1);
  size_t len1 = strnlen_s(s_MbErrorBuffer, k_ErrorBufferLength - len - 1);

  strncpy_s(s_MbFormatBuffer, k_ErrorBufferLength, format, len);
  strncat_s(s_MbFormatBuffer, k_ErrorBufferLength, s_MbErrorBuffer, len1);
}

void error_handling_start() {
  InitializeCriticalSection(&s_CrashLock);
  SetThreadDescription(GetCurrentThread(), L"Main Thread");
}

void error_handling_stop() { DeleteCriticalSection(&s_CrashLock); }

static void crash_start() {
  EnterCriticalSection(&s_CrashLock);
  write_minidump();
}

static void crash_end() {
  print_stack();
  error_popup();
  console_free();
  ExitProcess(11);
}

void crash(const char *format, ...) {
  crash_start();
  va_list args;
  va_start(args, format);

  set_error_va(format, args);

  va_end(args);

  crash_end();
}

void crash_code(int code, const char *format, ...) {
  crash_start();
  va_list args;
  va_start(args, format);

  append_return_code(format, code);
  set_error_va(s_MbFormatBuffer, args);

  va_end(args);
  crash_end();
}

void crash() { crash("Unknown"); }

void crash_windows(const char *format, ...) {
  DWORD lastErr = GetLastError();

  crash_start();
  va_list args;
  va_start(args, format);

  append_windows_message(format, lastErr);
  set_error_va(s_MbFormatBuffer, args);

  va_end(args);
  crash_end();
}

void crash_windows_code(int code, const char *format, ...) {
  DWORD lastErr = GetLastError();

  crash_start();
  va_list args;
  va_start(args, format);

  append_return_code(format, code);

  strncpy_s(s_MbErrorBuffer, k_ErrorBufferLength, s_MbFormatBuffer,
            k_ErrorBufferLength - 1);

  append_windows_message(s_MbErrorBuffer, lastErr);
  set_error_va(s_MbFormatBuffer, args);

  va_end(args);

  crash_end();
}

void crash_windows() { crash_windows("Unknown"); }

void crash_windows_hresult(HRESULT result) {
  crash_start();
  DWORD error = HRESULT_CODE(result);

  append_windows_message("Unknown Error", error);
  set_error(s_MbFormatBuffer);

  crash_end();
}

void crash_windows_hresult(HRESULT result, const char *format, ...) {
  crash_start();
  DWORD error = HRESULT_CODE(result);

  va_list args;
  va_start(args, format);

  append_windows_message(format, error);
  set_error_va(s_MbFormatBuffer, args);

  va_end(args);

  crash_end();
}