#pragma once

struct WindowInfo {
  const char *name = nullptr;
  unsigned int width = 0;
  unsigned int height = 0;
};

struct GameInfo {
  WindowInfo window;
};
