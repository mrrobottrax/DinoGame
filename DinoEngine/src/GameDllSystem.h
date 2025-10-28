#pragma once

#include "callbacks.h"

class GameDllSystem {
public:
  bool is_initialized() const;

  void load_game(const wchar_t *gameName);

  GameInfo GameInfo{};

#define GAME_CALLBACK(returnType, name) name##_t name##{};
  GAME_CALLBACKS_LIST
#undef GAME_CALLBACK

private:
  HMODULE m_GameModule{};
  bool m_IsInitialized{};
};

inline GameDllSystem g_GameDllSystem{};

inline bool GameDllSystem::is_initialized() const { return m_IsInitialized; }