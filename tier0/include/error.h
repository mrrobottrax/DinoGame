#pragma once

typedef int error_t;

void print_stack();
void set_error();
void set_error(const char *format, ...);
void set_windows_error();
void set_windows_error(const char *format, ...);
void set_windows_error(HRESULT result);
void set_windows_error(HRESULT result, const char *format, ...);
const char *get_error();
void free_error();

enum {
  T0_SUCCESS = 0,
  SUCCESS = 0,
  T0_ERROR,
  T0_ERROR_WINDOWS,
  T0_OUT_OF_MEMORY,
};

// Propogate errors through a function
#define CHECK(x)                                                               \
  {                                                                            \
    if (error_t error = x != T0_SUCCESS) {                                     \
      return error;                                                            \
    }                                                                          \
  }

// No runtime impact so static asserts are always active
#define _ASSERT_GLUE(a, b) a##b
#define ASSERT_GLUE(a, b) _ASSERT_GLUE(a, b)

#define STATIC_ASSERT(expression)                                              \
  enum { ASSERT_GLUE(g_assert_fail, __LINE__) = 1 / (int)(!!(expression)) }

#define THROW(...)                                                             \
  {                                                                            \
    if (IsDebuggerPresent()) {                                                 \
      __debugbreak();                                                          \
    }                                                                          \
    set_error("File: " __FILE__                                                \
              "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);            \
    print_stack();                                                             \
    return T0_ERROR_WINDOWS;                                                   \
  }

#define THROW_WIN(...)                                                         \
  {                                                                            \
    if (IsDebuggerPresent()) {                                                 \
      __debugbreak();                                                          \
    }                                                                          \
    set_windows_error("File: " __FILE__                                        \
                      "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);    \
    print_stack();                                                             \
    return T0_ERROR_WINDOWS;                                                   \
  }

#define ASSERT_WIN(result, ...)                                                \
  {                                                                            \
    if (SUCCEEDED(result)) {                                                   \
    } else {                                                                   \
      if (IsDebuggerPresent()) {                                               \
        __debugbreak();                                                        \
      }                                                                        \
      set_windows_error(result,                                                \
                        "Assertion Failed\r\nFile: " __FILE__                  \
                        "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);  \
      print_stack();                                                           \
      return T0_ERROR_WINDOWS;                                                 \
    }                                                                          \
  }

#ifndef NO_ASSERTS

#define ASSERT(expression, ...)                                                \
  {                                                                            \
    if (expression) {                                                          \
    } else {                                                                   \
      if (IsDebuggerPresent()) {                                               \
        __debugbreak();                                                        \
      }                                                                        \
      set_error("Assertion Failed\r\nFile: " __FILE__                          \
                "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);          \
      print_stack();                                                           \
      return T0_ERROR_WINDOWS;                                                 \
    }                                                                          \
  }

#else

#define ASSERT(expression, ...)

#endif
