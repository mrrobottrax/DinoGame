#include "pch.h"

void load_main_scene();
void load_settings_menu();
void open_settings_menu();

GAME_API void load_game_menu() {
  console_log("Loading menu...");
  load_settings_menu();

  UIMenu *pMenu = (UIMenu *)entity_spawn("ui_menu");
  pMenu->open();

  UITextButton *pStartButton = (UITextButton *)entity_spawn("ui_button");
  pStartButton->set_parent(pMenu);
  pStartButton->set_text("START");
  pStartButton->set_on_click(load_main_scene);

  UITextButton *pSettingsButton = (UITextButton *)entity_spawn("ui_button");
  pStartButton->set_parent(pMenu);
  pStartButton->set_text("SETTINGS");
  pStartButton->set_on_click(open_settings_menu);
}