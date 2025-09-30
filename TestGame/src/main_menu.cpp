#include "pch.h"

#include <dgui/screen.h>

GAME_API void load_main_menu() {
  console_log("Loading main menu");

  DGUI_ColoredPanel *mainMenuCentred = new DGUI_ColoredPanel();
  dgui_get_top_panel()->add_child(mainMenuCentred);
  mainMenuCentred->Anchor[0] = 0.5f;
  mainMenuCentred->Anchor[1] = 0;
  mainMenuCentred->Flags |= DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y;
  mainMenuCentred->set_position_dimensions(0, 0, 800, 0, 0.5f, 0);
  mainMenuCentred->set_color(1, 0, 0, 0.3f);

  DGUI_ColoredPanel *image = new DGUI_ColoredPanel();
  mainMenuCentred->add_child(image);
  image->Anchor[0] = 0.5f;
  image->Anchor[1] = 1;
  image->set_position_dimensions(0, -100, 600, 400, 0.5f, 0);
  image->set_color(1, 1, 0, 0.5f);
}