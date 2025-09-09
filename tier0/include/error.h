#pragma once

typedef int error_t;

void set_error(const char *format, ...);
void set_windows_error(const char *format, ...);
const char *get_error();
void free_error();

enum {
  T0_SUCCESS = 0,
  SUCCESS = 0,
  T0_ERROR,
  T0_ERROR_WINDOWS,
};

#define THROW(...)                                                             \
  set_error(__VA_ARGS__);                                                      \
  return T0_ERROR_WINDOWS;

#define THROW_WIN(...)                                                         \
  set_windows_error(__VA_ARGS__);                                              \
  return T0_ERROR_WINDOWS;
