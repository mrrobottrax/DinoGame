#include "pch.h"

#include "Engine.h"

#pragma warning(push)
#pragma warning(disable : 4100)
int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
             _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {

  t0_start();

  int nArgv;
  LPWSTR *argv = CommandLineToArgvW(lpCmdLine, &nArgv);

  g_Engine.parse_argv(argv, nArgv);

  LocalFree(argv);

  g_Engine.start();
  g_Engine.loop();
  g_Engine.stop();

  t0_stop();

  return 0;
}
#pragma warning(pop)