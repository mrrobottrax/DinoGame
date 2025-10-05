#include "pch.h"

#include "memory.h"

void memory_start_debug() {
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
}

void memory_check_leaks() {
  _CrtDumpMemoryLeaks();
#ifdef T0_CONSOLE
  system("pause");
#endif
}