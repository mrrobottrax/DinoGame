#include "pch.h"

#include "colors.h"

GAME_API GameInfo get_game_info() {
  return {
      .WindowName = "Editor",
  };
}

GAME_API void game_start() {
  console_log("Editor init");

  // UI_ColoredPanel *bg = ui_create<UI_ColoredPanel>();
  // bg->Flags |=
  //     UI_PANEL_FLAG_SUBTRACTIVE_SIZE_W | UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H;
  // bg->set_color(EDITOR_COLOR_BACKGROUND);

  UI_Grid *vGrid = ui_create<UI_Grid>();
  vGrid->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_WH;
  vGrid->v_split(30, UI_GRID_FLAG_ABSOLUTE_SIZE);

  UI_ColoredPanel *header = ui_create<UI_ColoredPanel>(vGrid);
  header->set_color(EDITOR_COLOR_MAIN);
  header->set_dimensions(200, 200);

  // UI_Grid *hGrid = ui_create<UI_Grid>();
  // hGrid->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_WH;
  // hGrid->h_split(40, UI_GRID_FLAG_ABSOLUTE_SIZE);

  // UI_ColoredPanel *toolbar = ui_create<UI_ColoredPanel>(hGrid);
  // toolbar->set_color(EDITOR_COLOR_MAIN);

  // UI_Grid *vpGrid = ui_create<UI_Grid>(hGrid);
  // vpGrid->h_split(0.5f, UI_GRID_FLAG_RELATIVE_SIZE);
  // vpGrid->v_split(0.5f, UI_GRID_FLAG_RELATIVE_SIZE);

  // UI_ColoredPanel *viewport = ui_create<UI_ColoredPanel>(vpGrid);
  // viewport->set_color(1, 0, 0, 0.5f);

  // UI_ColoredPanel *top = ui_create<UI_ColoredPanel>(vpGrid);
  // top->set_color(0, 1, 0, 0.5f);

  // UI_ColoredPanel *front = ui_create<UI_ColoredPanel>(vpGrid);
  // front->set_color(0, 0, 1, 0.5f);

  // UI_ColoredPanel *side = ui_create<UI_ColoredPanel>(vpGrid);
  // side->set_color(0, 1, 1, 0.5f);

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