#pragma once

#include "IEntity.h"

class DINO_API BaseCamera : IEntity {
public:
  vec3_t Position;
  quat_t Rotation;

  virtual void spawn() override;
};