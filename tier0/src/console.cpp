#include "pch.h"

#include "console.h"
#include "error.h"

error_t console_create() {
  if (!AllocConsole()) {
    THROW_WIN("AllocConsole failed");
  }

  FILE *stream;
  freopen_s(&stream, "CONOUT$", "w", stdout);
  freopen_s(&stream, "CONOUT$", "r", stdin);
  freopen_s(&stream, "CONOUT$", "w", stderr);

  SetConsoleOutputCP(CP_UTF8);

  return T0_SUCCESS;
}

error_t console_free() {
  FreeConsole();
  return T0_SUCCESS;
}