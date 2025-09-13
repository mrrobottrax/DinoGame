#pragma once

class Engine {
public:
  void init();
  void loop();
  void stop();
};

inline Engine g_Engine;