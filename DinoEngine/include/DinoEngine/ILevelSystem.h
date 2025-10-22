#pragma once

class ILevelSystem {
public:
  virtual bool queue_level_change(const char *levelName) = 0;
};

DINO_API extern ILevelSystem *g_ILevelSystem;