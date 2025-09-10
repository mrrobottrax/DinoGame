#include "pch.h"

#include "error.h"

#include "console.h"
#include "console_private.h"
#include "memory.h"

error_t t0_init() {
  setlocale(LC_ALL, ".UTF8");

  CHECK(console_create());
  memory_start_debug();

  return SUCCESS;
}

error_t t0_stop() {
  free_error();

  console_free_filebuffer();
  memory_check_leaks();
  CHECK(console_free());

  return SUCCESS;
}