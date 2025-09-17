#include "pch.h"

GAME_API GameInfo get_game_info() {
  return {
      .windowName = "Test Game",
  };
}

GAME_API void load_main_menu() {
  console_log("Loading main menu");

  DGUI_Panel *mainMenuPanel = new DGUI_Panel();
  dgui_get_top_panel()->add_child(mainMenuPanel);
  mainMenuPanel->set_position(100, 100);
  mainMenuPanel->set_size(100, 100);
  
  DGUI_Panel *testChild = new DGUI_Panel();
  mainMenuPanel->add_child(testChild);
  testChild->set_position(150, 150);
  testChild->set_size(100, 100);
}