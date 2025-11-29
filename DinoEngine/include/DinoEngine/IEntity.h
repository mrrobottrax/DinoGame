#pragma once

class IEntity {
public:
  virtual void spawn() = 0;
  virtual void update() = 0;
};