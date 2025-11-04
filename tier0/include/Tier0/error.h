#pragma once

typedef int code_t;

T0_API void crash();
T0_API void crash(const char *format, ...);
T0_API void crash_code(int code, const char *format, ...);
T0_API void crash_windows();
T0_API void crash_windows(const char *format, ...);
T0_API void crash_windows_code(int code, const char *format, ...);
T0_API void crash_windows_hresult(HRESULT result);
T0_API void crash_windows_hresult(HRESULT result, const char *format, ...);

#define ASSUME(expression, ...) __assume(expression);

#define MAKE_ERROR(Group, Major, Minor) (1##Group##Major##Minor)

#define CODE_SUCCESS(code) (code == 0)

#define DEBUG_BREAK_ALWAYS()                                                   \
  if (IsDebuggerPresent()) {                                                   \
    __debugbreak();                                                            \
  } else {                                                                     \
  }

#ifdef _DEBUG
#define DEBUG_BREAK() DEBUG_BREAK_ALWAYS()
#else
#define DEBUG_BREAK()
#endif

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
    ASSUME(0);                                                                 \
  }

#define CRASH_IMMEDIATE()                                                      \
  {                                                                            \
    if (IsDebuggerPresent()) {                                                 \
      __debugbreak();                                                          \
    }                                                                          \
    TerminateProcess(GetCurrentProcess(), 1);                                  \
  }

#define CRASH_WIN(...)                                                         \
  {                                                                            \
    if (IsDebuggerPresent()) {                                                 \
      __debugbreak();                                                          \
    }                                                                          \
    crash_windows("File: " __FILE__                                            \
                  "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);        \
    ASSUME(0);                                                                 \
  }

// Always asserts not stripped out in release builds. Use for critical error
// checking.
#define CHECK_CODE(expression, returnVal, ...)                                 \
  {                                                                            \
    code_t code = expression;                                                  \
    if (code == 0) {                                                           \
    } else {                                                                   \
      DEBUG_BREAK();                                                           \
      console_log(#expression " failed: %i", code);                            \
      console_log_debug(#expression                                            \
                        " failed\r\nFile: " __FILE__                           \
                        "\r\nLine:" STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);   \
      return returnVal;                                                        \
    }                                                                          \
  }

#define PROPAGATE_CODE(expression, ...)                                        \
  {                                                                            \
    code_t code = expression;                                                  \
    if (code == 0) {                                                           \
    } else {                                                                   \
      return code;                                                             \
    }                                                                          \
  }

#define ASSERT_RETURN(expression, returnVal, ...)                              \
  {                                                                            \
    if (expression) {                                                          \
    } else {                                                                   \
      DEBUG_BREAK();                                                           \
      console_error_debug(                                                     \
          #expression " failed. Returning " #returnVal "\r\nFile: " __FILE__   \
                      "\r\nLine:" STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);     \
      return returnVal;                                                        \
    }                                                                          \
  }

#define ASSERT_WIN_ALWAYS(expression, ...)                                     \
  {                                                                            \
    HRESULT result = expression;                                               \
    if (SUCCEEDED(result)) {                                                   \
    } else {                                                                   \
      DEBUG_BREAK_ALWAYS();                                                    \
      crash_windows_hresult(                                                   \
          result,                                                              \
          #expression " failed.\r\nFile: " __FILE__                            \
                      "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);    \
      ASSUME(0);                                                               \
    }                                                                          \
  }

#define ASSERT_WIN_EXP_ALWAYS(expression, ...)                                 \
  {                                                                            \
    if (expression) {                                                          \
    } else {                                                                   \
      DEBUG_BREAK_ALWAYS();                                                    \
      crash_windows("Assertion Failed: " #expression "\r\nFile: " __FILE__     \
                    "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);      \
      ASSUME(0);                                                               \
    }                                                                          \
  }

#define ASSERT_WIN_CODE_ALWAYS(expression, ...)                                \
  {                                                                            \
    code_t code = expression;                                                  \
    if (code == 0) {                                                           \
    } else {                                                                   \
      DEBUG_BREAK_ALWAYS();                                                    \
      crash_windows_code(code, #expression                                     \
                         " failed.\r\nFile: " __FILE__                         \
                         "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__); \
      ASSUME(0);                                                               \
    }                                                                          \
  }

#define ASSERT_ALWAYS(expression, ...)                                         \
  {                                                                            \
    if (expression) {                                                          \
    } else {                                                                   \
      DEBUG_BREAK_ALWAYS();                                                    \
      crash("Assertion Failed: " #expression "\r\nFile: " __FILE__             \
            "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);              \
      ASSUME(0);                                                               \
    }                                                                          \
  }

#define ASSERT_CODE_ALWAYS(expression, ...)                                    \
  {                                                                            \
    code_t code = expression;                                                  \
    if (code == 0) {                                                           \
    } else {                                                                   \
      DEBUG_BREAK_ALWAYS();                                                    \
      crash_code(code, #expression                                             \
                 " failed.\r\nFile: " __FILE__                                 \
                 "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);         \
      ASSUME(0);                                                               \
    }                                                                          \
  }

#ifndef NO_ASSERTS

#define ASSERT_WIN(...) ID(ASSERT_WIN_ALWAYS(__VA_ARGS__))
#define ASSERT_WIN_EXP(...) ID(ASSERT_WIN_EXP_ALWAYS(__VA_ARGS__))
#define ASSERT_WIN_CODE(...) ID(ASSERT_WIN_CODE_ALWAYS(__VA_ARGS__))

#define ASSERT(...) ID(ASSERT_ALWAYS(__VA_ARGS__))

#ifndef NO_SLOW_ASSERTS

#define ASSERT_WIN_SLOW(...) ID(ASSERT_WIN_ALWAYS(__VA_ARGS__))
#define ASSERT_WIN_EXP_SLOW(...) ID(ASSERT_WIN_EXP_ALWAYS(__VA_ARGS__))
#define ASSERT_WIN_CODE_SLOW(...) ID(ASSERT_WIN_CODE_ALWAYS(__VA_ARGS__))

#define ASSERT_SLOW(...) ID(ASSERT_ALWAYS(__VA_ARGS__))
#define ASSERT_CODE_SLOW(...) ID(ASSERT_CODE_ALWAYS(__VA_ARGS__))

#else

#define ASSERT_WIN_SLOW(expr, ...) ASSUME(expr);
#define ASSERT_WIN_EXP_SLOW(expr, ...) ASSUME(expr);
#define ASSERT_WIN_CODE_SLOW(expr, ...) ASSUME(expr);

#define ASSERT_SLOW(expr, ...) ASSUME(expr);
#define ASSERT_CODE_SLOW(expr, ...) ASSUME(expr);

#endif

#else

#define ASSERT_WIN(expr, ...) ASSUME(expr);
#define ASSERT_WIN_EXP(expr, ...) ASSUME(expr);
#define ASSERT_WIN_CODE(expr, ...) ASSUME(expr);

#define ASSERT(expr, ...) ASSUME(expr);

#define ASSERT_WIN_SLOW(expr, ...) ASSUME(expr);
#define ASSERT_WIN_EXP_SLOW(expr, ...) ASSUME(expr);
#define ASSERT_WIN_CODE_SLOW(expr, ...) ASSUME(expr);

#define ASSERT_SLOW(expr, ...) ASSUME(expr);
#define ASSERT_CODE_SLOW(expr, ...) ASSUME(expr);

#endif

// Assert that a lock is really not needed.
// This class will throw if the lock ever turns out to be needed.
// NOTE: To be cheap, this does not use memory barriers. But this only has to
// work once so it's fine.
class UnnecessaryLock {
#ifndef NO_ASSERTS
  volatile bool m_locked;

public:
  void acquire(const char *file, size_t line) {
    if (!m_locked) {
    } else {
      if (IsDebuggerPresent()) {
        __debugbreak();
      }
      crash("Unnecessary Lock was necessary.\r\nFile: %s\r\nLine: %zu", file,
            line);
    }
    m_locked = true;
  }

  void release(const char *file, size_t line) {
    if (m_locked) {
    } else {
      if (IsDebuggerPresent()) {
        __debugbreak();
      }
      crash("Unnecessary Lock was necessary.\r\nFile: %s\r\nLine: %zu", file,
            line);
    }
    m_locked = false;
  }
#endif
};

class UnnecessaryLockJanitor {
#ifndef NO_ASSERTS
  UnnecessaryLock *m_pLock;
  const char *m_file;
  size_t m_line;

public:
  explicit UnnecessaryLockJanitor(UnnecessaryLock &lock, const char *file,
                                  size_t line)
      : m_pLock(&lock), m_file(file), m_line(line) {
    m_pLock->acquire(m_file, m_line);
  }

  ~UnnecessaryLockJanitor() { m_pLock->release(m_file, m_line); }
#endif
};

#ifndef NO_ASSERTS

#define BEGIN_ASSERT_LOCK_NOT_NECESSARY(L) (L).acquire(__FILE__, __LINE__)
#define END_ASSERT_LOCK_NOT_NECESSARY(L) (L).release(__FILE__, __LINE__)
#define ASSERT_LOCK_NOT_NECESSARY(J, L)                                        \
  UnnecessaryLockJanitor J(L, __FILE__, __LINE__)

#else

#define BEGIN_ASSERT_LOCK_NOT_NECESSARY(L)
#define END_ASSERT_LOCK_NOT_NECESSARY(L)
#define ASSERT_LOCK_NOT_NECESSARY(J, L)

#endif
