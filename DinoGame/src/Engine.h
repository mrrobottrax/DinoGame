#pragma once

class Engine {
public:
  error_t init();
  error_t loop();
  error_t stop();
};

inline Engine *g_pEngine;