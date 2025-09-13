#include "pch.h"

#include "Engine.h"

#pragma warning(push)
#pragma warning(disable : 4100)
int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
             _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {

  t0_init();
  g_pEngine = new Engine();

  g_pEngine->init();
  g_pEngine->loop();
  g_pEngine->stop();

  delete g_pEngine;
  t0_stop();

  return 0;
}
#pragma warning(pop)