#include "pch.h"

#include "Engine.h"

error_t Engine::init() {
  THROW_WIN("TEST %s %d", "blah", 57);
  return SUCCESS;
}
error_t Engine::loop() { return SUCCESS; }
error_t Engine::stop() { return SUCCESS; }