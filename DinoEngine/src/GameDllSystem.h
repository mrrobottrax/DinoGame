#pragma once

#include "callbacks.h"

#define MCALLBACK(name) name##_ptr name

class GameDllSystem {
public:
  void load_game(const wchar_t *gameName);

private:
  HMODULE m_GameModule;

public:
  GameInfo GameInfo;

  // callbacks
  MCALLBACK(get_game_info);
  MCALLBACK(load_main_menu);
};

#undef MCALLBACK

inline GameDllSystem g_GameDllSystem{};