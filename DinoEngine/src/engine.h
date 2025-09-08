#pragma once

#include "game_info.h"

class WindowManager;
class InputManager;
class AssetManager;

class Engine {
private:
  char *m_gameName;
  HMODULE m_gameModule;

public:
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