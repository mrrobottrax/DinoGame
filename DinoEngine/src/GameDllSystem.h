#pragma once

#include "callbacks.h"

class GameDllSystem {
public:
  void load_game(const wchar_t *gameName);

private:
  HMODULE m_GameModule;

public:
  GameInfo GameInfo;

#define GAME_CALLBACK(returnType, name) name##_t name
  GAME_CALLBACKS_LIST
#undef GAME_CALLBACK
};

inline GameDllSystem g_GameDllSystem{};