#include "pch.h"

static Asset_Texture s_LogoPng;

GAME_API GameInfo get_game_info() {
  return GameInfo{
      .WindowName = "Voxel Game",
  };
}

GAME_API void game_start() {
  console_log("Voxels Started");
}