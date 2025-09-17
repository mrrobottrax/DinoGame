#include "pch.h"

GAME_API GameInfo get_game_info() {
  return {
      .windowName = "Test Game",
  };
}

GAME_API void load_main_menu() {
  console_log("Loading main menu");

  DGUI_ColoredPanel *mainMenuPanel = new DGUI_ColoredPanel();
  dgui_get_top_panel()->add_child(mainMenuPanel);
  mainMenuPanel->set_position(100, 100);
  mainMenuPanel->set_size(100, 100);
  mainMenuPanel->set_color(1, 0, 0, 1);

  DGUI_ColoredPanel *testChild = new DGUI_ColoredPanel();
  mainMenuPanel->add_child(testChild);
  testChild->set_position(50, 50);
  testChild->set_size(200, 110);
  testChild->set_color(0, 1, 0, 0.5f);
}