#pragma once

void crash();
void crash(const char *format, ...);
void crash_windows();
void crash_windows(const char *format, ...);
void crash_windows(HRESULT result);
void crash_windows(HRESULT result, const char *format, ...);

enum {
  T0_SUCCESS = 0,
  SUCCESS = 0,
  T0_ERROR,
  T0_ERROR_WINDOWS,
  T0_OUT_OF_MEMORY,
};

// No runtime impact so static asserts are always active
#define _ASSERT_GLUE(a, b) a##b
#define ASSERT_GLUE(a, b) _ASSERT_GLUE(a, b)

#define STATIC_ASSERT(expression)                                              \
  enum { ASSERT_GLUE(g_assert_fail, __LINE__) = 1 / (int)(!!(expression)) }

#define CRASH(...)                                                             \
  {                                                                            \
    if (IsDebuggerPresent()) {                                                 \
      __debugbreak();                                                          \
    }                                                                          \
    crash("File: " __FILE__                                                    \
          "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);                \
  }

#define CRASH_WIN(...)                                                         \
  {                                                                            \
    if (IsDebuggerPresent()) {                                                 \
      __debugbreak();                                                          \
    }                                                                          \
    crash_windows("File: " __FILE__                                            \
                  "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);        \
  }

#ifndef NO_ASSERTS

#define ASSERT_WIN(result, ...)                                                \
  {                                                                            \
    if (SUCCEEDED(result)) {                                                   \
    } else {                                                                   \
      if (IsDebuggerPresent()) {                                               \
        __debugbreak();                                                        \
      }                                                                        \
      crash_windows(result,                                                    \
                    "Windows Assertion Failed\r\nFile: " __FILE__              \
                    "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);      \
    }                                                                          \
  }

#define ASSERT(expression, ...)                                                \
  {                                                                            \
    if (expression) {                                                          \
    } else {                                                                   \
      if (IsDebuggerPresent()) {                                               \
        __debugbreak();                                                        \
      }                                                                        \
      crash("Assertion Failed: " #expression "\r\nFile: " __FILE__             \
            "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);              \
    }                                                                          \
  }

#ifndef NO_SLOW_ASSERTS
#define ASSERT_SLOW(...) ASSERT(__VA_ARGS__)
#else
#define ASSERT_SLOW(...)
#endif

#else

#define ASSERT_WIN(...)
#define ASSERT(...)
#define ASSERT_SLOW(...)

#endif
