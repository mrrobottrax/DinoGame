#pragma once

#include "ILevelSystem.h"

class LevelSystem : public ILevelSystem {
public:
  virtual bool queue_level_change(const char *levelName) override {
    return false;
  }

  void unload_immediate();
};

inline LevelSystem g_LevelSystem{};