#include "pch.h"

#include "Engine.h"

#include "AssetSystem.h"
#include "EntitySystem.h"
#include "GameDllSystem.h"
#include "LevelSystem.h"
#include "RenderingSystem.h"
#include "UISystem.h"
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

void Engine::start() {
  ASSERT_ALWAYS(m_GameName);

  ResourceLoader_SetupInfo resourceLoaderSetup{};
  ASSERT_ALWAYS(ResourceLoader_setup(&resourceLoaderSetup, nullptr));

  g_GameDllSystem.load_game(m_GameName);

  g_WindowSystem.start(g_GameDllSystem.GameInfo.WindowName, 1280, 720,
                       g_GameDllSystem.GameInfo.CanResizeWindow);
  g_RenderingSystem.start();
  g_AssetSystem.start();
  g_UISystem.start();
  g_EntitySystem.start();

  g_WindowSystem.show_finally();

  g_GameDllSystem.game_start();
}

void Engine::stop() {
  g_LevelSystem.unload_immediate();

  g_EntitySystem.stop();
  g_UISystem.stop();
  g_AssetSystem.stop();
  g_RenderingSystem.stop();
  g_WindowSystem.stop();
  ResourceLoader_close();

  free(m_GameName);
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
    }

    g_RenderingSystem.frame();
  }
}