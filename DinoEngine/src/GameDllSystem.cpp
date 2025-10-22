#include "pch.h"

#include "GameDllSystem.h"

void GameDllSystem::load_game(const wchar_t *gameName) {
  m_GameModule = LoadLibraryExW(gameName, NULL,
                                0); // TODO: LOAD_LIBRARY_SEARCH_APPLICATION_DIR

  ASSERT_ALWAYS(m_GameModule != NULL, "Failed to load game DLL");

#define GAME_CALLBACK(returnType, name)                                        \
  name = (name##_t)GetProcAddress(m_GameModule, name##_name);                  \
  ASSERT_ALWAYS(name != nullptr, "Could not find callback.")

  GAME_CALLBACKS_LIST

#undef GAME_CALLBACK

  GameInfo = get_game_info();
}
