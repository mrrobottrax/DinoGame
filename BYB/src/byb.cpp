#include "pch.h"

static Asset_Texture s_LogoPng;

GAME_API GameInfo get_game_info() {
  return GameInfo{
      .WindowName = "Voxel Game",
  };
}

GAME_API void game_start() {
  console_log("Voxels Started");

  s_LogoPng = g_IAssetSystem->load_texture("business.png");

  UI_ColoredPanel *bg = ui_create<UI_ColoredPanel>();
  bg->Flags |= UI_PANEL_FLAG_SUBTRACTIVE_SIZE_WH;
  bg->set_color(1, 1, 1, 1);

  UI_Image *logo = ui_create<UI_Image>();
  logo->Texture = &s_LogoPng;
  logo->set_anchor(0.5f, 0.5f);
  logo->set_pivot(0.5f, 0.5f);
  logo->set_dimensions(1000, 600);
}