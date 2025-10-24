#include "pch.h"

GAME_API GameInfo get_game_info() {
  return GameInfo{
      .WindowName = "Build Your Battles!",
  };
}

GAME_API void game_start() { console_log("BYB Started"); }