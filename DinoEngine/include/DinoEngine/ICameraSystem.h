#pragma once

class BaseCamera;

class ICameraSystem {
public:
  virtual BaseCamera *get_main_camera() = 0;
};

DINO_API extern ICameraSystem *g_ICameraSystem;