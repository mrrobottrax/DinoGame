#include "pch.h"

GAME_API void load_main_menu() {
  console_log("Loading main menu");

  IAssetSystem *assetSystem = get_asset_system_interface();
  GPUImage logo = assetSystem->load_png("EngineLogo.png");

  DGUI_ColoredPanel *mainMenuCentred = new DGUI_ColoredPanel();
  dgui_get_top_panel()->add_child(mainMenuCentred);
  mainMenuCentred->Anchor[0] = 0.5f;
  mainMenuCentred->Anchor[1] = 0;
  mainMenuCentred->Flags |= DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y;
  mainMenuCentred->set_position_dimensions(0, 0, 800, 0, 0.5f, 0);
  mainMenuCentred->set_color(1, 0, 0, 0.3f);

  DGUI_Image *image = new DGUI_Image();
  mainMenuCentred->add_child(image);
  image->TextureHandle = logo.get_handle();
  image->TextureHeap = logo.get_heap();
  image->Anchor[0] = 0.5f;
  image->Anchor[1] = 1;
  image->set_position_dimensions(0, -80, 600, 400, 0.5f, 1);
}