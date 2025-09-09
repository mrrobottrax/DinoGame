#pragma once

#include "game_callbacks.h"

class WindowManager;
class InputManager;
class AssetManager;

#define GAME_PTR(name) name##_ptr_t name = nullptr;

class Engine {
private:
  char *m_gameName;
  HMODULE m_gameModule;

public:
  GAME_PTR(get_game_info)
  GAME_PTR(load_game_menu)

  // systems
  WindowManager *pWindowManager;
  InputManager *pInputManager;
  AssetManager *pAssetManager;

public:
  ~Engine();

public:
  void parse_cmd_args(const wchar_t *const *argv, int numArgs);
  void init();
  void loop();
  void stop();
  void toggle_reload();

private:
  void load_game();
  void unload_game();
};

inline Engine *g_engine;