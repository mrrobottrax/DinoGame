#include "pch.h"

#include "engine.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                    _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
  try {
    console_create();
    memory_start_debug();

    Engine *engine = new Engine{};
    g_engine = engine;

    int numArgs;
    LPWSTR *argv = CommandLineToArgvW(lpCmdLine, &numArgs);
    engine->parse_cmd_args(argv, numArgs);
    LocalFree(argv);

    engine->init();
    engine->loop();
    engine->stop();

    delete engine;
    engine = nullptr;
    g_engine = nullptr;

    memory_check_leaks();
    console_free();
  } catch (Exception &e) {
    console_log_error("CRASH!!\nReason: %s", e.message());

    // convert to wide
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, e.message(), -1, NULL, 0);
    wchar_t *wideBuffer = (wchar_t *)malloc(wideLen * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, e.message(), -1, wideBuffer, wideLen);

    MessageBox(NULL, wideBuffer, L"ERROR", MB_OK | MB_ICONERROR);

    free(wideBuffer);
  } catch (...) {
    console_log_error("CRASH!!\nReason: Unknown");
    MessageBox(NULL, L"Unknown error", L"ERROR", MB_OK | MB_ICONERROR);
  }

  return 0;
}