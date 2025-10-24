#include "pch.h"

#include "colors.h"

GAME_API GameInfo get_game_info() {
  return {
      .WindowName = "Editor",
  };
}

GAME_API void game_start() {
  console_log("Editor init");

  UI_Grid *vGrid = ui_create<UI_Grid>();
  vGrid->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_WH;
  vGrid->v_split(30, UI_GRID_FLAG_ABSOLUTE_SIZE);

  UI_ColoredPanel *header = ui_create<UI_ColoredPanel>(vGrid);
  header->set_color(EDITOR_COLOR_MAIN);

  UI_Grid *hGrid = ui_create<UI_Grid>(vGrid);
  hGrid->h_split(40, UI_GRID_FLAG_ABSOLUTE_SIZE);

  UI_ColoredPanel *toolbar = ui_create<UI_ColoredPanel>(hGrid);
  toolbar->set_color(EDITOR_COLOR_MAIN);

  UI_Grid *vpGrid = ui_create<UI_Grid>(hGrid);
  vpGrid->h_split(0.5f, UI_GRID_FLAG_RELATIVE_SIZE);
  vpGrid->v_split(0.5f, UI_GRID_FLAG_RELATIVE_SIZE);

  UI_ColoredPanel *viewport = ui_create<UI_ColoredPanel>(vpGrid);
  viewport->set_color(1, 0, 0, 0.5f);

  UI_ColoredPanel *top = ui_create<UI_ColoredPanel>(vpGrid);
  top->set_color(0, 1, 0, 0.5f);

  UI_ColoredPanel *front = ui_create<UI_ColoredPanel>(vpGrid);
  front->set_color(0, 0, 1, 0.5f);

  UI_ColoredPanel *side = ui_create<UI_ColoredPanel>(vpGrid);
  side->set_color(0, 1, 1, 0.5f);
}