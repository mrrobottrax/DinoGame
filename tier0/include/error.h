#pragma once

void crash();
void crash(const char *format, ...);
void crash_windows();
void crash_windows(const char *format, ...);
void crash_windows(HRESULT result);
void crash_windows(HRESULT result, const char *format, ...);

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
    return;                                                                    \
  }

#define CRASH_IMMEDIATE()                                                      \
  {                                                                            \
    if (IsDebuggerPresent()) {                                                 \
      __debugbreak();                                                          \
    }                                                                          \
    TerminateProcess(GetCurrentProcess(), 1);                                  \
    return;                                                                    \
  }

#define CRASH_WIN(...)                                                         \
  {                                                                            \
    if (IsDebuggerPresent()) {                                                 \
      __debugbreak();                                                          \
    }                                                                          \
    crash_windows("File: " __FILE__                                            \
                  "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);        \
    return;                                                                    \
  }

// Always asserts not stripped out in release builds. Use for critical error
// checking.

#define ASSERT_WIN_ALWAYS(result, ...)                                         \
  {                                                                            \
    if (SUCCEEDED(result)) {                                                   \
    } else {                                                                   \
      if (IsDebuggerPresent()) {                                               \
        __debugbreak();                                                        \
      }                                                                        \
      crash_windows(#result,                                                   \
                    " failed.\r\nFile: " __FILE__                              \
                    "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);      \
      return;                                                                  \
    }                                                                          \
  }

#define ASSERT_ALWAYS(expression, ...)                                         \
  {                                                                            \
    if (expression) {                                                          \
    } else {                                                                   \
      if (IsDebuggerPresent()) {                                               \
        __debugbreak();                                                        \
      }                                                                        \
      crash("Assertion Failed: " #expression "\r\nFile: " __FILE__             \
            "\r\nLine: " STRINGIZE(__LINE__) "\r\n" __VA_ARGS__);              \
      return;                                                                  \
    }                                                                          \
  }

#ifndef NO_ASSERTS

#define ASSERT_WIN(...) ASSERT_WIN_ALWAYS(__VA_ARGS__)

#define ASSERT(...) ASSERT_ALWAYS(__VA_ARGS__)

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
