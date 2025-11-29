#include "pch.h"

#include "ObserverCamera.h"

static Asset_Texture s_LogoPng;

GAME_API GameInfo get_game_info() {
  return GameInfo{
      .WindowName = "Voxel Game",
  };
}

GAME_API void game_start() {
  console_log("Voxels Started");
  ObserverCamera *camera = entity_create<ObserverCamera>();

  ASSERT_ALWAYS(camera != nullptr);

  camera->Position = vec3_t{8, 8, -10};
  camera->Rotation = quat_identity();
}