#include "pch.h"

#include "control.h"

#include "error.h"
#include "error_private.h"

#include "console.h"
#include "console_private.h"
#include "memory.h"

T0_API void t0_start() {
  setlocale(LC_ALL, ".UTF8");

  error_handling_start();

  console_create();
  memory_start_debug();
}

T0_API void t0_stop() {
  error_handling_stop();

  memory_check_leaks();
  console_free();
}