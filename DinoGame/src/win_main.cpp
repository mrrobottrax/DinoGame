#include "pch.h"

#include "Engine.h"

static void handle_error() {
  if (get_error()) {
    int wcLen = MultiByteToWideChar(CP_UTF8, 0, get_error(), -1, NULL, 0);
    wchar_t *wcErr = (wchar_t *)malloc(sizeof(wchar_t) * wcLen);
    MultiByteToWideChar(CP_UTF8, 0, get_error(), -1, wcErr, wcLen);
    MessageBoxExW(NULL, wcErr, L"Error", MB_OK | MB_ICONERROR, 0);

    free(wcErr);
    free_error();
  } else {
    MessageBoxExW(NULL, L"Unknown Error", L"Error", MB_OK | MB_ICONERROR, 0);
  }

  delete g_pEngine;
  t0_stop();
}

#define CRASH_ON_ERROR(...)                                                    \
  if (error_t error = __VA_ARGS__ != T0_SUCCESS) {                             \
    handle_error();                                                            \
    return error;                                                              \
  }

#pragma warning(push)
#pragma warning(disable : 4100)
int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
             _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {

  CRASH_ON_ERROR(t0_init());

  g_pEngine = new Engine();

  CRASH_ON_ERROR(g_pEngine->init());
  CRASH_ON_ERROR(g_pEngine->loop());
  CRASH_ON_ERROR(g_pEngine->stop());

  delete g_pEngine;
  t0_stop();

  return SUCCESS;
}
#pragma warning(pop)