#pragma once

void console_line();
void console_print(const char format[], ...);
void console_println(const char format[], ...);
void console_print_va(const char format[], va_list args);
void console_println_va(const char format[], va_list args);
void console_log(const char format[], ...);
void console_warn(const char format[], ...);
void console_error(const char format[], ...);

inline void console_log(unsigned int n) { console_log("%u", n); }
inline void console_log(int n) { console_log("%d", n); }
inline void console_log(unsigned short n) { console_log("%hu", n); }
inline void console_log(short n) { console_log("%h", n); }
inline void console_log(void *p) { console_log("%p", p); }
inline void console_log(float f) { console_log("%f", f); }
