#include "pch.h"

#include "Engine.h"

int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
             _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
  error_t error;
  if ((error = t0_init() != T0_SUCCESS) ||
      (g_pEngine = new Engine(), error = g_pEngine->init() != T0_SUCCESS) ||
      (error = g_pEngine->loop() != T0_SUCCESS) ||
      (error = g_pEngine->stop() != T0_SUCCESS)) {

    int wcLen = MultiByteToWideChar(CP_UTF8, 0, get_error(), -1, NULL, 0);
    wchar_t *wcErr = (wchar_t *)malloc(sizeof(wchar_t) * wcLen);
    MultiByteToWideChar(CP_UTF8, 0, get_error(), -1, wcErr, wcLen);
    MessageBoxExW(NULL, wcErr, L"Error", MB_OK | MB_ICONERROR, 0);

    t0_stop();
    return error;
  }

  delete g_pEngine;

  t0_stop();
  return SUCCESS;
}