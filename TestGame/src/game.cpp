#include "pch.h"

#include "main_menu.h"

GAME_API GameInfo get_game_info() {
  return {
      .windowName = "Test Game",
  };
}

GAME_API void load_main_menu() {
  console_log("Loading main menu");
  main_menu_load();
}