#include "pch.h"

#include "error.h"

#include "console.h"
#include "console_private.h"
#include "memory.h"

error_t t0_init() {
  setlocale(LC_ALL, ".UTF8");
  memory_start_debug();
  if (error_t error = console_create() != T0_SUCCESS) {
    return error;
  }

  return SUCCESS;
}

error_t t0_stop() {
  memory_check_leaks();
  console_free();

  return SUCCESS;
}