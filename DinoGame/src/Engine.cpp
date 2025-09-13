#include "pch.h"

#include "Engine.h"

#include "WindowSystem.h"

void Engine::init() { g_WindowSystem.init("DinoEngine Test"); }

void Engine::loop() {
  MSG msg;
  while (true) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (msg.message == WM_QUIT) {
        return;
      }
    }
  }
}

void Engine::stop() { g_WindowSystem.stop(); }