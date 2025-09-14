#include "pch.h"

#include "console.h"
#include "console_private.h"
#include "error.h"
#include "error_private.h"

__declspec(thread) static char *s_LastError = nullptr;
static CRITICAL_SECTION s_CrashLock;

static void free_error() {
  free(s_LastError);
  s_LastError = nullptr;
}

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

  SYMBOL_INFO *symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256, 1);
  if (symbol == 0)
    return;
  symbol->MaxNameLen = 255;
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

  free(symbol);
}

static void error_popup() {
  int wcLen = MultiByteToWideChar(CP_UTF8, 0, s_LastError, -1, NULL, 0);
  wchar_t *wcErr = (wchar_t *)malloc(sizeof(wchar_t) * wcLen);
  MultiByteToWideChar(CP_UTF8, 0, s_LastError, -1, wcErr, wcLen);
  MessageBoxExW(NULL, wcErr, L"Error", MB_OK | MB_ICONERROR, 0);

  free(wcErr);
}

static void set_error_va(const char *format, va_list args) {
  free_error();

  va_list args_required;
  va_copy(args_required, args);
  size_t required = _vscprintf(format, args_required);
  va_end(args_required);

  console_line();
  console_println("!! CRASH !!");

  if (!format)
    return;

  s_LastError = (char *)malloc(required + 1);
  if (s_LastError) {
    va_list args_log;
    va_copy(args_log, args);
    console_println_va(format, args_log);
    va_end(args_log);

    va_list args_vsnprintf;
    va_copy(args_vsnprintf, args);
    if (vsnprintf_s(s_LastError, required + 1, _TRUNCATE, format,
                    args_vsnprintf) < 0) {
      console_error("Formatting failure");
    }
    va_end(args_vsnprintf);
  }
}

static char *add_windows_message_to_format(const char *format, DWORD error) {
  LPWSTR wideErr = nullptr;
  DWORD wideErrLen = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                                        FORMAT_MESSAGE_IGNORE_INSERTS |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    NULL, error, 0, (LPWSTR)&wideErr, 0, NULL);

  char *mbErr = nullptr;
  size_t mbErrLen = 0;
  if (wideErr != nullptr) {
    mbErrLen = WideCharToMultiByte(CP_UTF8, 0, wideErr, wideErrLen + 1, NULL, 0,
                                   NULL, NULL);

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

  if (mbErr == 0 || finalFormat == 0) {
    return nullptr;
  }

  strncpy_s(finalFormat, len + affixLen + mbErrLen + 1, format, len);
  strncpy_s(finalFormat + len, affixLen + mbErrLen + 1, affix, affixLen);
  strncpy_s(finalFormat + len + affixLen, mbErrLen + 1, mbErr, mbErrLen);

  return finalFormat;
}

void error_handling_init() {
  InitializeCriticalSection(&s_CrashLock);
  SetThreadDescription(GetCurrentThread(), L"Main Thread");
}

void error_handling_stop() {
  DeleteCriticalSection(&s_CrashLock);
  free_error();
}

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

void crash() { crash("Unknown"); }

void crash_windows(const char *format, ...) {
  crash_start();
  va_list args;
  va_start(args, format);

  char *finalFormat = add_windows_message_to_format(format, GetLastError());
  set_error_va(finalFormat, args);
  free(finalFormat);

  va_end(args);

  crash_end();
}

void crash_windows() { crash_windows("Unknown"); }

void crash_windows(HRESULT result) {
  crash_start();
  DWORD error = HRESULT_CODE(result);

  char *finalFormat = add_windows_message_to_format("Unknown Error", error);
  crash(finalFormat);
  free(finalFormat);

  crash_end();
}

void crash_windows(HRESULT result, const char *format, ...) {
  crash_start();
  DWORD error = HRESULT_CODE(result);

  va_list args;
  va_start(args, format);

  char *finalFormat = add_windows_message_to_format(format, error);
  set_error_va(finalFormat, args);
  free(finalFormat);

  va_end(args);

  crash_end();
}