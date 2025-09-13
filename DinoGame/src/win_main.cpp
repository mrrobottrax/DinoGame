#include "pch.h"

#include "Engine.h"

#pragma warning(push)
#pragma warning(disable : 4100)
int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
             _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {

  t0_init();

  g_Engine.init();
  g_Engine.loop();
  g_Engine.stop();

  t0_stop();

  return 0;
}
#pragma warning(pop)