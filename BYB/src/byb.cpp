#include "pch.h"

GAME_API GameInfo get_game_info() {
  return GameInfo{
      .WindowName = "Build Your Battles!",
  };
}

GAME_API void game_start() {
  console_log("BYB Started");

  UI_ColoredPanel *bg = ui_create<UI_ColoredPanel>();
  bg->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_WH;
  bg->set_color(1, 1, 1, 1);

  UI_Image *logo = ui_create<UI_Image>();
  logo->set_texture("Logo.png");
  logo->set_anchor(0.5f, 0.5f);
  logo->set_pivot(0.5f, 0.5f);
  logo->set_dimensions(800, 600);
}