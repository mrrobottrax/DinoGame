#include "pch.h"

#include "colors.h"

GAME_API GameInfo get_game_info() {
  return {
      .WindowName = "Editor",
  };
}

GAME_API void game_start() {
  console_log("Editor init");

  UI_ColoredPanel *mainMenuPanel = ui_create<UI_ColoredPanel>();
  mainMenuPanel->Flags |=
      UI_PANEL_FLAG_SUBTRACTIVE_SIZE_X | UI_PANEL_FLAG_ABSOLUTE_SIZE_Y;
  mainMenuPanel->set_anchor(0, 1);
  mainMenuPanel->set_pivot(0, 1);
  mainMenuPanel->set_position(0, 0);
  mainMenuPanel->set_dimensions(0, 30);
  mainMenuPanel->set_color(COLOR_MAIN);

  /*
  g_ILevelSystem->set_skybox("Menu_Sky.png");

  UI_ColoredPanel &mainMenuCentred = ui_create<UI_ColoredPanel>();
  mainMenuCentred->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y;
  mainMenuCentred->set_anchor(0.5f, 0);
  mainMenuCentred->set_pivot(0.5f, 0);
  mainMenuCentred->set_position(0, 0);
  mainMenuCentred->set_dimensions(800, 0);
  mainMenuCentred->set_color(1, 1, 1, 0.1f);

  UI_Image &image = ui_create<UI_Image>(mainMenuCentred);
  image->set_texture("EngineLogo.png");
  image->Anchor[0] = 0.5f;
  image->Anchor[1] = 1;
  image->set_position_dimensions(0, -80, 900, 400, 0.5f, 1);

  DyanmicProp &cube = entity_spawn<DyanmicProp>();
  cube->set_position(0, 0, 0);
  cube->set_model("MenuCube.gltf");
  cube->set_animation("spin");

  Camera &camera = entity_spawn<Camera>();
  camera.set_position(0, -5, 0);

  playsound_loop("menu.wav", SND_MUSIC);
  */
}