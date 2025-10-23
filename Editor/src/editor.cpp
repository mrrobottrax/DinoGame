#include "pch.h"

#include "colors.h"

GAME_API GameInfo get_game_info() {
  return {
      .WindowName = "Editor",
  };
}

GAME_API void game_start() {
  console_log("Editor init");

  UI_ColoredPanel *bg = ui_create<UI_ColoredPanel>();
  bg->Flags |=
      UI_PANEL_FLAG_SUBTRACTIVE_SIZE_W | UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H;
  bg->set_color(EDITOR_COLOR_BACKGROUND);

  UI_ColoredPanel *header = ui_create<UI_ColoredPanel>();
  header->set_absolute();
  header->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_W;
  header->set_anchor(0, 1);
  header->set_pivot(0, 1);
  header->set_dimensions(0, 30);
  header->set_color(EDITOR_COLOR_MAIN);

  UI_ColoredPanel *toolbar = ui_create<UI_ColoredPanel>();
  toolbar->set_absolute();
  toolbar->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H;
  toolbar->set_dimensions(40, 31);
  toolbar->set_color(EDITOR_COLOR_MAIN);

  UI_Panel *editorWindow = ui_create<UI_Panel>();
  editorWindow->set_absolute();
  editorWindow->Flags |=
      UI_PANEL_FLAG_SUBTRACTIVE_SIZE_W | UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H;
  editorWindow->set_position(41, 0);
  editorWindow->set_dimensions(41, 31);

  UI_ColoredPanel *viewport = ui_create<UI_ColoredPanel>(editorWindow);
  viewport->set_relative();
  viewport->set_position(0, 0.5f);
  viewport->set_dimensions(0.5f, 0.5f);
  viewport->set_color(1, 0, 0, 0.5f);

  UI_ColoredPanel *top = ui_create<UI_ColoredPanel>(editorWindow);
  top->set_relative();
  top->set_position(0.5f, 0.5f);
  top->set_dimensions(0.5f, 0.5f);
  top->set_color(0, 1, 0, 0.5f);

  UI_ColoredPanel *front = ui_create<UI_ColoredPanel>(editorWindow);
  front->set_relative();
  front->set_position(0, 0);
  front->set_dimensions(0.5f, 0.5f);
  front->set_color(0, 0, 1, 0.5f);

  UI_ColoredPanel *side = ui_create<UI_ColoredPanel>(editorWindow);
  side->set_relative();
  side->set_position(0.5f, 0);
  side->set_dimensions(0.5f, 0.5f);
  side->set_color(0, 1, 1, 0.5f);

  /*
  g_ILevelSystem->set_skybox("Menu_Sky.png");

  UI_ColoredPanel &mainMenuCentred = ui_create<UI_ColoredPanel>();
  mainMenuCentred->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H;
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