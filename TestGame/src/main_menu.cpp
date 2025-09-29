#include "pch.h"

GAME_API void load_main_menu(uint32_t width, uint32_t height) {
  console_log("Loading main menu");

  DGUI_ColoredPanel *mainMenuCentred = new DGUI_ColoredPanel();
  dgui_get_top_panel()->add_child(mainMenuCentred);
  mainMenuCentred->set_position(width / 2 - 250, 0);
  mainMenuCentred->set_size(500, height);
  mainMenuCentred->set_color(1, 0, 0, 0.25f);
}