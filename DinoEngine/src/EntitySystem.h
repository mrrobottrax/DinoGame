#pragma once

#include "IEntitySystem.h"

class EntitySystem : public IEntitySystem {
public:
  void start();
  void stop();
};

inline EntitySystem g_EntitySystem{};