#pragma once

#include "ICameraSystem.h"

class CameraSystem : public ICameraSystem {
public:
  void set_main_camera(BaseCamera *camera);
  virtual BaseCamera *get_main_camera() override;

private:
  BaseCamera *m_MainCamera{};
};

inline CameraSystem g_CameraSystem{};

inline BaseCamera *CameraSystem::get_main_camera() { return m_MainCamera; }
inline void CameraSystem::set_main_camera(BaseCamera *camera) {
  m_MainCamera = camera;
}