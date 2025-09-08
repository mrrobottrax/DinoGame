#include "pch.h"

#include "asset_manager.h"
#include "engine.h"
#include "input_manager.h"
#include "keyvalues.h"
#include "window_manager.h"

Engine::~Engine() {
  free(m_gameName);
  m_gameName = nullptr;

  stop();
}

void Engine::init() {
  // set default game dll
  if (m_gameName == nullptr) {
    constexpr const char k_defaultGame[] = "DinoGame";
    constexpr size_t len = sizeof(k_defaultGame) - 1;

    m_gameName = (char *)malloc(len + 1);
    strncpy_s(m_gameName, len + 1, k_defaultGame, len);
  }

  pWindowManager = new WindowManager{};
  pInputManager = new InputManager{};
  pAssetManager = new AssetManager{};

  load_game();
}

void Engine::stop() {
  unload_game();

  delete pWindowManager;
  pWindowManager = nullptr;

  delete pInputManager;
  pInputManager = nullptr;

  delete pAssetManager;
  pAssetManager = nullptr;
}

void Engine::toggle_reload() {
  if (m_gameModule != NULL) {
    unload_game();
  } else {
    load_game();
  }
}

void Engine::parse_cmd_args(const wchar_t *const *argv, int numArgs) {
  const wchar_t *pKey = nullptr;
  for (int i = 0; i < numArgs; ++i) {
    const wchar_t *pArg = argv[i];

    if (pArg[0] == L'-') {
      pKey = pArg;
    } else if (pKey != nullptr) {
      if (wcsncmp(pKey, L"-game", 4) == 0) {
        int mbLen =
            WideCharToMultiByte(CP_UTF8, 0, pArg, -1, NULL, 0, NULL, NULL);
        m_gameName = (char *)malloc(mbLen);
        WideCharToMultiByte(CP_UTF8, 0, pArg, -1, m_gameName, mbLen, NULL,
                            NULL);
      }
    }
  }
}

void Engine::loop() {
  MSG msg;
  while (true) {
    // process all messages
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (msg.message == WM_QUIT) {
        return;
      }
    }

    pInputManager->process_input();

    // run frame code
  }
}

constexpr const char k_failedToLoad[] = "Failed to load game function \"%s\"";

#define GET_CALLBACK(func)                                                     \
  func = (func##_ptr)GetProcAddress(m_gameModule, k_##func##_name);            \
  if (func == nullptr) {                                                       \
    console_log_error(k_failedToLoad, #func);                                  \
    unload_game();                                                             \
    return;                                                                    \
  }

void Engine::load_game() {
  unload_game();

  console_log("Loading game: %s", m_gameName);
  int wcDllLen = MultiByteToWideChar(CP_UTF8, 0, m_gameName, -1, NULL, 0);
  wchar_t *wcDll = (wchar_t *)malloc(wcDllLen * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, m_gameName, -1, wcDll, wcDllLen);
  m_gameModule =
      LoadLibraryExW(wcDll, NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
  if (m_gameModule == NULL) {
    throw WindowsException("Failed to load game DLL");
  }
  free(wcDll);

  pAssetManager->set_default_context(&pAssetManager->gameContext);

  Asset *gameInfoAsset = pAssetManager->precache("game_info.txt");
  Asset *gamePreloadAsset = pAssetManager->precache("game_preload.txt");

  pAssetManager->asset_barrier(gameInfoAsset);
  pAssetManager->asset_barrier(gamePreloadAsset);

  KVObject gameInfoKv =
      KVObject::parse(&gameInfoAsset->data, gameInfoAsset->length);
  KVObject gamePreloadKv =
      KVObject::parse(&gamePreloadAsset->data, gamePreloadAsset->length);

  // read game info file
  GameInfo gameInfo{};
  gameInfo.menuLevel = gameInfoKv["menuLevel"].cstr();
  gameInfo.pauseLevel = gameInfoKv["pauseLevel"].cstr();

  KVObject &windowKv = gameInfoKv["window"];
  gameInfo.window.height = windowKv["height"].to_uint();
  gameInfo.window.width = windowKv["width"].to_uint();
  gameInfo.window.name = windowKv["name"].cstr();

  // read preload file
  KVObject &preload = gamePreloadKv["preload"];
  for (int i = 0, l = (int)preload.length(); i < l; ++i) {
    pAssetManager->precache(preload[i].cstr());
  }

  pAssetManager->context_barrier(&pAssetManager->gameContext);

  console_log("Game Name: %s", gameInfo.window.name);
  console_log("Game Width: %u", gameInfo.window.width);
  console_log("Game Height: %u", gameInfo.window.height);
  console_log("Menu Level: %s", gameInfo.menuLevel);

  pWindowManager->update_window(&gameInfo.window);
  pInputManager->register_raw_input(pWindowManager->hWnd);

  pAssetManager->unload(gameInfoAsset);
  pAssetManager->unload(gamePreloadAsset);
}

void Engine::unload_game() {
  if (m_gameModule != NULL) {
    console_log("Unloading game...");

    pAssetManager->unload_context(&pAssetManager->gameContext);

    FreeLibrary(m_gameModule);
    m_gameModule = NULL;

    GameInfo gameInfo = {
        .window =
            {
                .name = "Unloaded...",
            },
    };
    pWindowManager->update_window(&gameInfo.window);
  }
}