#include "pch.h"

#include "error.h"
#include "error_private.h"

#include "console.h"
#include "console_private.h"
#include "memory.h"

void t0_init() {
  setlocale(LC_ALL, ".UTF8");

  error_handling_init();

  console_create();
  memory_start_debug();
}

void t0_stop() {
  free_error();

  console_free_filebuffer();
  memory_check_leaks();
  console_free();
}