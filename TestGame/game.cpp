#include "pch.h"

GameInfo get_game_info() {
  return {
      .windowName = "Test Game",
  };
}

// static UIMenu *s_SettingsMenu;

//static void start_game() { console_log("Game start"); }
//
//static void quit_game() { console_log("Game quit"); }
//
//static void open_settings() { g_UISystem.open_menu(s_SettingsMenu); }

void load_main_menu() {
  console_log("Loading main menu");

  //// main menu
  // UIMenu *mainMenu = g_UISystem.allocate_menu();

  //// settings menu
  // UIMenu *settingsMenu = g_UISystem.allocate_menu();
  // s_SettingsMenu = settingsMenu;

  //// populate main menu
  //{
  //  UISprite *title = mainMenu.allocate_element<UISprite>("menu_title.png");
  //  title->anchor[0] = 0.5f;
  //  title->anchor[1] = 0.5f;
  //  title->pivot[0] = 0.5f;
  //  title->pivot[1] = 0.5f;
  //  title->offset[1] = 50;

  //  UITextButton *start =
  //  mainMenu.allocate_element<UITextButton>("#btn_start"); start->anchor[0] =
  //  0.5f; start->anchor[1] = 0.5f; start->pivot[0] = 0.5f; start->pivot[1] =
  //  0.5f; start->offset[1] = 10; start->onClick = start_game;

  //  UITextButton *settings =
  //      mainMenu.allocate_element<UITextButton>("#btn_settings");
  //  settings->anchor[0] = 0.5f;
  //  settings->anchor[1] = 0.5f;
  //  settings->pivot[0] = 0.5f;
  //  settings->pivot[1] = 0.5f;
  //  settings->offset[1] = 0;
  //  settings->onClick = open_settings;

  //  UITextButton *quit = mainMenu.allocate_element<UITextButton>("#btn_quit");
  //  quit->anchor[0] = 0.5f;
  //  quit->anchor[1] = 0.5f;
  //  quit->pivot[0] = 0.5f;
  //  quit->pivot[1] = 0.5f;
  //  quit->offset[1] = -10;
  //  quit->onClick = quit_game;
  //}

  //// populate settings menu
  //{
  //  UIText *title = mainMenu.allocate_element<UIText>("#ui_settings");
  //  title->textScale = 2;
  //  title->anchor[0] = 0.5f;
  //  title->anchor[1] = 0.5f;
  //  title->pivot[0] = 0.5f;
  //  title->pivot[1] = 0.5f;
  //  title->offset[1] = 50;

  //  UIText *placeholder = mainMenu.allocate_element<UIText>("Placeholder
  //  text"); placeholder->anchor[0] = 0.5f; placeholder->anchor[1] = 0.5f;
  //  placeholder->pivot[0] = 0.5f;
  //  placeholder->pivot[1] = 0.5f;
  //  placeholder->offset[1] = 0;
  //}

  // g_UISystem.open_menu(mainMenu);
}