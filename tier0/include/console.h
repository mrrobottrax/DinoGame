#pragma once

inline void console_line() { printf("\n"); }

inline void console_log(const char message[], ...) {
  printf("[LOG] ");

  va_list args;
  va_start(args, message);

  vprintf(message, args);

  va_end(args);

  printf("\n");
}

inline void console_log_va(const char message[], va_list args) {
  printf("[LOG] ");

  vprintf(message, args);

  printf("\n");
}

inline void console_log_warn(const char message[], ...) {
  printf("[WARNING] ");

  va_list args;
  va_start(args, message);

  vprintf(message, args);

  va_end(args);

  printf("\n");
}

inline void console_log_warn_va(const char message[], va_list args) {
  printf("[WARNING] ");

  vprintf(message, args);

  printf("\n");
}

inline void console_log_error(const char message[], ...) {
  printf("[ERROR] ");

  va_list args;
  va_start(args, message);

  vprintf(message, args);

  va_end(args);

  printf("\n");
}

inline void console_log_error_va(const char message[], va_list args) {
  printf("[ERROR] ");

  vprintf(message, args);

  printf("\n");
}

#ifdef CPP
inline void console_log(unsigned int n) { console_log("%u", n); }
inline void console_log(int n) { console_log("%d", n); }
inline void console_log(unsigned short n) { console_log("%hu", n); }
inline void console_log(short n) { console_log("%h", n); }
inline void console_log(void *p) { console_log("%p", p); }
inline void console_log(float f) { console_log("%f", f); }
#endif
