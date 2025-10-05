#include "pch.h"

#include "GameDllSystem.h"

void GameDllSystem::load_game(const wchar_t *gameName) {
  m_GameModule = LoadLibraryExW(gameName, NULL,
                                0); // TODO: LOAD_LIBRARY_SEARCH_APPLICATION_DIR

  ASSERT_ALWAYS(m_GameModule != NULL, "Failed to load game DLL");

  // get callbacks
#define GET_CALLBACK(name)                                                     \
  name = (name##_ptr)GetProcAddress(m_GameModule, name##_name);                \
  ASSERT_ALWAYS(name != nullptr, "Could not find callback.")

  GET_CALLBACK(get_game_info);
  GET_CALLBACK(load_main_menu);

  GameInfo = get_game_info();
}
