#include "pch.h"

#include "Engine.h"

#include "GameDllSystem.h"
#include "RenderingSystem.h"
#include "WindowSystem.h"

constexpr size_t k_MaxArgvLength = 256;

void Engine::parse_argv(wchar_t **argv, int nArgs) {
  console_log("CMD Arguments: %d", nArgs);
  for (int i = 0; i < nArgs; ++i) {
    console_log("\t%ls", argv[i]);
  }

  const wchar_t *key = nullptr;

  for (int i = 0; i < nArgs; ++i) {
    if (argv[i][0] == L'-') {
      key = argv[i];
      continue;
    }

    if (key == nullptr)
      continue;

    if (wcscmp(key, L"-game") == 0) {
      size_t argvLen = wcsnlen_s(argv[i], k_MaxArgvLength);
      size_t byteSize = (argvLen + 1) * sizeof(wchar_t);
      m_GameName = (wchar_t *)malloc(byteSize);
      ASSERT_ALWAYS(m_GameName);
      memcpy_s(m_GameName, byteSize, argv[i], byteSize);
    }
  }
}

void Engine::init() {
  if (m_GameName == nullptr) {
    constexpr wchar_t k_DefaultGame[] = L"TestGame";
    m_GameName = (wchar_t *)malloc(sizeof(k_DefaultGame));
    ASSERT_ALWAYS(m_GameName);
    memcpy_s(m_GameName, sizeof(k_DefaultGame), k_DefaultGame,
             sizeof(k_DefaultGame));
  }

  g_GameDllSystem.load_game(m_GameName);

  g_WindowSystem.init(g_GameDllSystem.gameInfo.windowName);
  g_RenderingSystem.init();

  g_GameDllSystem.load_main_menu();
}

void Engine::loop() {
  MSG msg;
  while (true) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (msg.message == WM_QUIT) {
        return;
      }

      g_RenderingSystem.frame();
    }
  }
}

void Engine::stop() {
  free(m_GameName);

  g_RenderingSystem.stop();
  g_WindowSystem.stop();
}