#include "pch.h"

#include "console.h"
#include "exception.h"

void console_create() {
  if (!AllocConsole()) {
    throw WindowsException("AllocConsole failed");
  }

  FILE *stream;
  freopen_s(&stream, "CONOUT$", "w", stdout);
  freopen_s(&stream, "CONOUT$", "r", stdin);
  freopen_s(&stream, "CONOUT$", "w", stderr);

  setlocale(LC_ALL, ".UTF8");
  SetConsoleOutputCP(CP_UTF8);
}

void console_free() { FreeConsole(); }