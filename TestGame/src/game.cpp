#include "pch.h"

GameInfo get_game_info() {
  return {
      .windowName = "Test Game",
  };
}

void load_main_menu() {
  console_log("Loading main menu");

  //DGUI_Panel *mainMenuPanel = new DGUI_Panel();
  //dgui_get_top_panel()->add_child(mainMenuPanel);
  //mainMenuPanel->set_position(0, 0);
  //mainMenuPanel->set_size(500, 500);
}